package app.zxtune.fs.dbhelpers;

import java.io.IOException;

public interface FetchCommand<T> {
  T fetchFromCache();
  T fetchFromRemote() throws IOException;
}
