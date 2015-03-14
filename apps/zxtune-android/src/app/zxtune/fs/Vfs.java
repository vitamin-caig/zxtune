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

import android.content.Context;
import app.zxtune.MainApplication;

public final class Vfs {

  private static VfsRoot rootSingleton;

  public synchronized static VfsRoot getRoot() {
    if (rootSingleton == null) {
      final VfsRootComposite composite = new VfsRootComposite();
      final Context appContext = MainApplication.getInstance();
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootZxtunes(appContext));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      composite.addSubroot(new VfsRootModland(appContext));
      composite.addSubroot(new VfsRootHvsc(appContext));
      composite.addSubroot(new VfsRootZxart(appContext));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
