/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import android.content.Context;
import android.util.Log;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final static String CACHE_DIR_NAME = "ftp.modland.com";
  private final static int MIN_CACHED_FILE_SIZE = 256;

  private final String cacheDir;
  private final Catalog remote;
  private final Database db;

  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = context.getCacheDir().getAbsolutePath() + File.separator + CACHE_DIR_NAME + File.separator;
    this.remote = remote;
    this.db = db;
    createCacheDir();
  }

  private void createCacheDir() {
    final File dir = new File(cacheDir);
    if (!dir.exists()) {
      dir.mkdirs();
    }
  }

  @Override
  public void queryAuthors(String filter, AuthorsVisitor visitor) throws IOException {
    final CountingAuthorsVisitor count = new CountingAuthorsVisitor(visitor);
    db.queryAuthors(filter, count);
    if (0 == count.get()) {
      Log.d(TAG, "Authors cache for filter '" + filter + "' is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthors(filter, new CachingAuthorsVisitor(count));
        transaction.succeed();
        Log.d(TAG, "Cached " + count.get() + " authors");
      } finally {
        transaction.finish();
      }
    }
  }

  @Override
  public Author queryAuthor(int id) throws IOException {
    Author res = db.queryAuthor(id);
    if (res == null) {
      Log.d(TAG, "No author id=" + id + " in cache. Query from remote");
      res = remote.queryAuthor(id);
      if (res != null) {
        Log.d(TAG, "Cache author id=" + id);
        db.addAuthor(res);
      }
    }
    return res;
  }

  @Override
  public void queryAuthorTracks(int authorId, TracksVisitor visitor) throws IOException {
    //TODO: make another way to check if cache is filled
    final CountingTracksVisitor count = new CountingTracksVisitor(visitor);
    db.queryAuthorTracks(authorId, count);
    if (0 == count.get()) {
      Log.d(TAG, "Tracks cache is empty for author=" + authorId);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthorTracks(authorId, new CachingTracksVisitor(count, authorId));
        transaction.succeed();
        Log.d(TAG, "Cached " + count.get() + " tracks");
      } finally {
        transaction.finish();
      }
    }
  }

  @Override
  public ByteBuffer getTrackContent(String id) throws IOException {
    final File cache = new File(cacheDir + id);
    try {
      if (cache.exists()) {
        Log.d(TAG, "Reading content of track " + id + " from cache");
        return readFrom(cache);
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to read from cache", e);
    }
    final ByteBuffer content = remote.getTrackContent(id);
    if (content.capacity() >= MIN_CACHED_FILE_SIZE) {
      Log.d(TAG, "Write content of track " + id + " to cache");
      writeTo(cache, content);
    } else {
      Log.d(TAG, "Do not cache suspicious file");
    }
    return content;
  }

  //TODO: remove C&P
  private ByteBuffer readFrom(File file) throws IOException {
    final FileInputStream stream = new FileInputStream(file);
    try {
      final FileChannel channel = stream.getChannel();
      try {
        return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
      } finally {
        channel.close();
      }
    } finally {
      stream.close();
    }
  }

  private void writeTo(File file, ByteBuffer data) {
    try {
      file.getParentFile().mkdirs();
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        final FileChannel chan = stream.getChannel();
        try {
          chan.write(data);
        } finally {
          chan.close();
        }
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "writeTo " + file.getAbsolutePath(), e);
      file.delete();
    }
  }

  private static class CountingAuthorsVisitor implements AuthorsVisitor {

    private final AuthorsVisitor delegate;
    private int count;

    CountingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      ++count;
    }

    final int get() {
      return count;
    }
  }

  private class CachingAuthorsVisitor implements AuthorsVisitor {

    private final AuthorsVisitor delegate;

    CachingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, "acceptAuthor()", e);
      }
    }
  }

  private static class CountingTracksVisitor implements TracksVisitor {

    private final TracksVisitor delegate;
    private int count;

    CountingTracksVisitor(TracksVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      ++count;
    }

    final int get() {
      return count;
    }
  }

  private class CachingTracksVisitor implements TracksVisitor {

    private final TracksVisitor delegate;
    private final int author;

    CachingTracksVisitor(TracksVisitor delegate, int author) {
      this.delegate = delegate;
      this.author = author;
    }

    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      try {
        db.addTrack(obj);
        db.addAuthorsTrack(obj, author);
      } catch (Exception e) {
        Log.d(TAG, "addTrack()", e);
      }
    }
  }
}
