package app.zxtune.fs.dbhelpers;

import androidx.annotation.NonNull;
import app.zxtune.fs.http.HttpObject;

import java.io.File;
import java.io.IOException;

public interface DownloadCommand {
  @NonNull
  File getCache() throws IOException;

  @NonNull
  HttpObject getRemote() throws IOException;
}
