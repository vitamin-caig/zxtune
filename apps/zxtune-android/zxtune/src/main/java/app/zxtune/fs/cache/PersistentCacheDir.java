package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.Log;
import app.zxtune.fs.Io;

final class PersistentCacheDir implements CacheDir {
  private static final String TAG = PersistentCacheDir.class.getName();

  private final File dir;

  PersistentCacheDir(File dir) {
    this.dir = dir;
  }

  @Nullable
  @Override
  public ByteBuffer findFile(String id) {
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
      if (file.getParentFile().mkdirs()) {
        Log.d(TAG, "Created cache dir");
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
  public CacheDir createNested(String id) {
    return new PersistentCacheDir(getSub(id));
  }

  private File getSub(String id) {
    return new File(dir, id);
  }
}
