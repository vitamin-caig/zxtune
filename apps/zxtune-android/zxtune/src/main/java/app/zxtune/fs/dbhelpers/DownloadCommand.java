package app.zxtune.fs.dbhelpers;

import java.io.File;
import java.io.IOException;

import app.zxtune.fs.http.HttpObject;

public interface DownloadCommand {
  File getCache() throws IOException;

  HttpObject getRemote() throws IOException;
}
