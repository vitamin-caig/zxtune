package app.zxtune.fs.dbhelpers;

import android.support.annotation.Nullable;

import java.io.IOException;

public interface FetchCommand<T> {
  // may return null if no cached object
  @Nullable
  T fetchFromCache();

  // return non-null or throw
  T fetchFromRemote() throws IOException;
}
