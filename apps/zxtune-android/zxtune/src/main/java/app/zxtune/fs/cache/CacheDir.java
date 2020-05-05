package app.zxtune.fs.cache;

import androidx.annotation.Nullable;

import java.io.File;

public interface CacheDir {

  @Nullable
  File find(String... ids);
}
