package app.zxtune.fs.cache;

import java.io.File;

final class StubCacheDir implements CacheDir {

  @Override
  public File find(String... ids) {
    return null;
  }
}
