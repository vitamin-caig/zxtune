package app.zxtune.fs.httpdir;

import android.net.Uri;

public interface Path {
  String getRemoteUrl();
  String getLocalId();
  Uri getUri();
  String getName();
  Path getParent();
  Path getChild(String name);
  boolean isEmpty();
  boolean isFile();
}
