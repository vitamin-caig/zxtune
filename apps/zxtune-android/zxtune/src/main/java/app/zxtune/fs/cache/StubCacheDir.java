package app.zxtune.fs.cache;

import androidx.annotation.Nullable;

import java.io.File;

final class StubCacheDir implements CacheDir {

  @Nullable
  @Override
  public File find(String... ids) {
    return null;
  }
}
