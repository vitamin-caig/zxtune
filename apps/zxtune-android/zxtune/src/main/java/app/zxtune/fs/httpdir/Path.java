package app.zxtune.fs.httpdir;

import android.net.Uri;

public interface Path {
  Uri getRemoteUri();
  String getLocalId();
  Uri getUri();
  String getName();
  Path getParent();
  Path getChild(String name);
  boolean isEmpty();
  boolean isFile();
}
