/**
 * @file
 * @brief VFS entry point
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import java.io.IOException;

import app.zxtune.MainApplication;

public final class Vfs {

  private static VfsRoot rootSingleton;


  public static VfsDir getRoot() throws IOException {
    return getRootInternal();
  }


  public static VfsObject resolve(Uri uri) throws IOException {
    final VfsObject res = getRootInternal().resolve(uri);
    if (res != null) {
      return res;
    } else {
      throw new IOException("Failed to resolve " + uri);
    }
  }

  private static synchronized VfsRoot getRootInternal() throws IOException {
    if (rootSingleton == null) {
      final VfsRootComposite composite = new VfsRootComposite();
      final Context appContext = MainApplication.getInstance();
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootNetwork(appContext));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
