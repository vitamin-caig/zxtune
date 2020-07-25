package app.zxtune.fs.cache;

import java.io.File;

final class CompositeCacheDir implements CacheDir {

  private final CacheDir[] delegates;

  CompositeCacheDir(CacheDir... delegates) {
    this.delegates = delegates;
  }

  @Override
  public File find(String... ids) {
    File result = null;
    for (CacheDir delegate : delegates) {
      final File res = delegate.find(ids);
      if (res != null) {
        if (res.isFile()) {
          return res;
        } else if (result == null) {
          result = res;
        }
      }
    }
    return result;
  }
}
