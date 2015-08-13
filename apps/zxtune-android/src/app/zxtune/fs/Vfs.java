/**
 *
 * @file
 *
 * @brief VFS entry point
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;

import android.content.Context;
import android.net.Uri;
import app.zxtune.MainApplication;

public final class Vfs {

  private static VfsRoot rootSingleton;
  
  public static VfsDir getRoot() {
    return getRootInternal();
  }
  
  public static VfsObject resolve(Uri uri) throws IOException {
    return getRootInternal().resolve(uri);
  }

  synchronized static VfsRoot getRootInternal() {
    if (rootSingleton == null) {
      final VfsRootComposite composite = new VfsRootComposite();
      final Context appContext = MainApplication.getInstance();
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootZxtunes(appContext));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      composite.addSubroot(new VfsRootModland(appContext));
      composite.addSubroot(new VfsRootHvsc(appContext));
      composite.addSubroot(new VfsRootZxart(appContext));
      composite.addSubroot(new VfsRootJoshw(appContext));
      composite.addSubroot(new VfsRootAmp(appContext));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
