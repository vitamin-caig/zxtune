package app.zxtune.fs.cache;

import androidx.annotation.Nullable;

import java.io.File;

final class PersistentCacheDir implements CacheDir {
  private static final String TAG = PersistentCacheDir.class.getName();

  private final File dir;

  PersistentCacheDir(File dir) {
    this.dir = dir;
  }

  @Nullable
  @Override
  public File find(String... ids) {
    File result = null;
    for (String id : ids) {
      final File file = getSub(id);
      if (file.isFile()) {
        return file;
      } else if (result == null && !file.isDirectory()) {
        result = file;
      }
    }
    return result;
  }

  @Override
  public CacheDir createNested(String id) {
    return new PersistentCacheDir(getSub(id));
  }

  private File getSub(String id) {
    return new File(dir, id);
  }
}
