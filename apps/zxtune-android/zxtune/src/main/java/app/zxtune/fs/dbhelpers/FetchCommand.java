package app.zxtune.fs.dbhelpers;

import androidx.annotation.Nullable;

import java.io.IOException;

public interface FetchCommand<T> {

  @Nullable
  T fetchFromCache();

  T updateCache() throws IOException;
}
