package app.zxtune.fs.dbhelpers;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.IOException;

public interface FetchCommand<T> {

  String getScope();

  @Nullable
  T fetchFromCache();

  @NonNull
  T updateCache() throws IOException;
}
