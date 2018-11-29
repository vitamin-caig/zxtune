package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import app.zxtune.Log;
import app.zxtune.io.Io;
import app.zxtune.io.TransactionalOutputStream;

final class PersistentCacheDir implements CacheDir {
  private static final String TAG = PersistentCacheDir.class.getName();

  private final File dir;

  PersistentCacheDir(File dir) {
    this.dir = dir;
  }

  @NonNull
  @Override
  public File findOrCreate(String... ids) throws IOException {
    File result = null;
    for (String id : ids) {
      final File file = getSub(id);
      if (file.isFile()) {
        return file;
      } else if (result == null && !file.isDirectory() && createParentDirFor(file)) {
        result = file;
      }
    }
    if (result != null) {
      return result;
    } else {
      throw new IOException("Failed to create cache");
    }
  }

  private static boolean createParentDirFor(@NonNull File file) {
    final File parent = file.getParentFile();
    return parent.isDirectory() || (parent.mkdirs() && parent.isDirectory());
  }

  @Nullable
  @Override
  public ByteBuffer findFile(String... ids) {
    for (String id : ids) {
      final ByteBuffer res = findFile(id);
      if (res != null) {
        return res;
      }
    }
    return null;
  }

  private ByteBuffer findFile(String id) {
    try {
      final File file = getSub(id);
      if (!file.isFile()) {
        Log.d(TAG, "No cached file %s", file.getAbsolutePath());
        return null;
      }
      Log.d(TAG, "Reading cached file %s", file.getAbsolutePath());
      return Io.readFrom(file);
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to read from cache");
    }
    return null;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    final File file = getSub(id);
    Log.d(TAG, "Write cached file %s", file.getAbsolutePath());
    try {
      if (!createParentDirFor(file)) {
        throw new IOException("Failed to create parent directory");
      }
      Io.writeTo(file, data);
      return Uri.fromFile(file);
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to write to %s", file.getAbsolutePath());
      if (!file.delete()) {
        Log.d(TAG, "Failed to delete %s", file);
      }
    }
    return Uri.EMPTY;
  }

  @Override
  public OutputStream createFile(String id) throws IOException {
    final File file = getSub(id);
    Log.d(TAG, "Create cached file stream %s", file.getAbsolutePath());
    return new TransactionalOutputStream(file);
  }

  @Override
  public CacheDir createNested(String id) {
    return new PersistentCacheDir(getSub(id));
  }

  private File getSub(String id) {
    return new File(dir, id);
  }
}
