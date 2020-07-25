package app.zxtune.fs.dbhelpers;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.analytics.Analytics;
import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.http.HttpObject;
import app.zxtune.io.Io;
import app.zxtune.io.TransactionalOutputStream;

public class CommandExecutor {

  private static final String TAG = CommandExecutor.class.getName();

  private final String id;

  public CommandExecutor(String id) {
    this.id = id;
  }

  public final void executeQuery(QueryCommand cmd) throws IOException {
    final Analytics.VfsTrace trace = Analytics.VfsTrace.create(id, cmd.getScope());
    final int action = executeQueryImpl(cmd);
    trace.send(action);
  }

  private int executeQueryImpl(QueryCommand cmd) throws IOException {
    if (cmd.getLifetime().isExpired()) {
      return refreshAndQuery(cmd);
    } else {
      cmd.queryFromCache();
      return Analytics.VFS_ACTION_CACHED_FETCH;
    }
  }

  private int refreshAndQuery(QueryCommand cmd) throws IOException {
    try {
      executeRefresh(cmd);
      cmd.queryFromCache();
      return Analytics.VFS_ACTION_REMOTE_FETCH;
    } catch (IOException e) {
      if (cmd.queryFromCache()) {
        return Analytics.VFS_ACTION_CACHED_FALLBACK;
      } else {
        throw e;
      }
    }
  }

  private void executeRefresh(QueryCommand cmd) throws IOException {
    final Transaction transaction = cmd.startTransaction();
    try {
      cmd.updateCache();
      cmd.getLifetime().update();
      transaction.succeed();
    } finally {
      transaction.finish();
    }
  }

  public final <T> T executeFetchCommand(FetchCommand<T> cmd) throws IOException {
    final Analytics.VfsTrace trace = Analytics.VfsTrace.create(id, cmd.getScope());
    final T cached = cmd.fetchFromCache();
    if (cached != null) {
      trace.send(Analytics.VFS_ACTION_CACHED_FETCH);
      return cached;
    }
    final T remote = cmd.updateCache();
    trace.send(Analytics.VFS_ACTION_REMOTE_FETCH);
    return remote;
  }

  public final ByteBuffer executeDownloadCommand(DownloadCommand cmd,
                                                 @Nullable ProgressCallback progress) throws IOException {
    final Analytics.VfsTrace trace = Analytics.VfsTrace.create(id, "file");
    int action = Analytics.VFS_ACTION_CACHED_FETCH;
    HttpObject remote = null;
    try {
      final File cache = cmd.getCache();
      final boolean isEmpty = cache.length() == 0;//also if not exists
      if (isEmpty || needUpdate(cache)) {
        try {
          remote = cmd.getRemote();
          if (isEmpty || needUpdate(cache, remote)) {
            Log.d(TAG, "Download %s to %s", remote.getUri(), cache.getAbsolutePath());
            download(remote, cache, progress);
            trace.send(Analytics.VFS_ACTION_REMOTE_FETCH);
          } else {
            Log.d(TAG, "Update timestamp of %s", cache.getAbsolutePath());
            Io.touch(cache);
          }
          return Io.readFrom(cache);
        } catch (IOException e) {
          Log.w(TAG, e, "Failed to update cache");
          if (!isEmpty) {
            Io.touch(cache);
          }
          action = Analytics.VFS_ACTION_CACHED_FALLBACK;
        }
      }
      if (cache.canRead()) {
        final ByteBuffer result = Io.readFrom(cache);
        trace.send(action);
        return result;
      }
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to load from cache");
    }
    if (remote == null) {
      remote = cmd.getRemote();
    }
    final ByteBuffer result = download(remote, progress);
    trace.send(Analytics.VFS_ACTION_REMOTE_FALLBACK);
    return result;
  }

  private static boolean needUpdate(File cache) {
    final long CACHE_CHECK_PERIOD_MS = 24 * 60 * 60 * 1000;
    final long lastModified = cache.lastModified();
    if (lastModified == 0) {
      return false;//not supported
    }
    return lastModified + CACHE_CHECK_PERIOD_MS < System.currentTimeMillis();
  }

  private static boolean needUpdate(File cache, HttpObject remote) {
    return remoteUpdated(cache, remote) || sizeChanged(cache, remote);
  }

  private static boolean remoteUpdated(File cache, HttpObject remote) {
    final long localLastModified = cache.lastModified();
    final TimeStamp remoteLastModified = remote.getLastModified();
    if (localLastModified != 0 && remoteLastModified != null && remoteLastModified.convertTo(TimeUnit.MILLISECONDS) > localLastModified) {
      Log.d(TAG, "Update outdated file: %d -> %d", localLastModified, remoteLastModified.convertTo(TimeUnit.MILLISECONDS));
      return true;
    } else {
      return false;
    }
  }

  private static boolean sizeChanged(File cache, HttpObject remote) {
    final long localSize = cache.length();
    final Long remoteSize = remote.getContentLength();
    if (remoteSize != null && remoteSize != localSize) {
      Log.d(TAG, "Update changed file: %d -> %d", localSize, remoteSize);
      return true;
    } else {
      return false;
    }
  }

  private void download(HttpObject remote, File cache, @Nullable ProgressCallback progress) throws IOException {
    checkAvailableSpace(remote, cache);
    final TransactionalOutputStream output = new TransactionalOutputStream(cache);
    try {
      final InputStream input = remote.getInput();
      final Long remoteSize = remote.getContentLength();
      final InputStream in = ProgressTrackingInput.wrap(input, remoteSize, progress);
      Io.copy(in, output);
      output.flush();
    } finally {
      output.close();
    }
  }

  private static void checkAvailableSpace(HttpObject remote, File cache) throws IOException {
    final File dir = cache.getParentFile();
    final long availSize = dir != null ? dir.getUsableSpace() : 0;
    if (availSize != 0) {
      final Long remoteSize = remote.getContentLength();
      if (remoteSize != null && remoteSize > availSize) {
        throw new IOException("No free space for cache");//TODO: localize
      }
    }
  }

  private static ByteBuffer download(HttpObject remote, @Nullable ProgressCallback progress) throws IOException {
    final Long remoteSize = remote.getContentLength();
    final InputStream input = remote.getInput();
    if (remoteSize != null) {
      final InputStream in = ProgressTrackingInput.wrap(input, remoteSize, progress);
      return Io.readFrom(in, remoteSize);
    } else {
      return Io.readFrom(input);
    }
  }

  private static class ProgressTrackingInput extends InputStream {
    private final InputStream delegate;
    private final ProgressCallback progress;
    private final int total;
    private long done;

    static InputStream wrap(InputStream in, @Nullable Long size, @Nullable ProgressCallback progress) {
      return size != null && progress != null
          ? new ProgressTrackingInput(in, progress, size)
          : in;
    }

    private ProgressTrackingInput(InputStream delegate, ProgressCallback progress, long total) {
      this.delegate = delegate;
      this.progress = progress;
      this.total = (int) (total / 1024);
    }

    @Override
    public void close() throws IOException {
      delegate.close();
    }

    @Override
    public int read() throws IOException {
      final int res = delegate.read();
      if (res != -1) {
        update(1);
      }
      return res;
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
      final int res = delegate.read(b, off, len);
      if (res >= 0) {
        update(res);
      }
      return res;
    }

    private void update(int len) {
      done += len;
      if (len >= 1024) {
        progress.onProgressUpdate((int) (done / 1024), total);
      }
    }
  }
}
