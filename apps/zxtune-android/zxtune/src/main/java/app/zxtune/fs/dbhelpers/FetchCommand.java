package app.zxtune.fs.dbhelpers;

import androidx.annotation.Nullable;

import java.io.IOException;

public interface FetchCommand<T> {

  String getScope();

  @Nullable
  T fetchFromCache();

  T updateCache() throws IOException;
}
