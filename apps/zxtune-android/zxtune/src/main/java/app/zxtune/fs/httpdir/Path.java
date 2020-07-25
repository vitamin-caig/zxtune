package app.zxtune.fs.httpdir;

import android.net.Uri;

import androidx.annotation.Nullable;

public interface Path {
  Uri[] getRemoteUris();
  String getLocalId();
  Uri getUri();
  String getName();
  @Nullable
  Path getParent();
  Path getChild(String name);
  boolean isEmpty();
  boolean isFile();
}
