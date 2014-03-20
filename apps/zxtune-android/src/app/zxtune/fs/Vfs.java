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

public final class Vfs {

  private static VfsRoot rootSingleton;

  public synchronized static VfsRoot createRoot(Context context) {
    if (rootSingleton == null) {
      final VfsRootComposite composite = new VfsRootComposite();
      final Context appContext = context.getApplicationContext();
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootZxtunes(appContext));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      composite.addSubroot(new VfsRootModland(appContext));
      composite.addSubroot(new VfsRootHvsc(appContext));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
