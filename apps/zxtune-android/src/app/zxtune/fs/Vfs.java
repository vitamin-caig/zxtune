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
      final HttpProvider http = new HttpProvider(appContext);
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootZxtunes(appContext, http));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      composite.addSubroot(new VfsRootModland(appContext, http));
      composite.addSubroot(new VfsRootHvsc(appContext, http));
      composite.addSubroot(new VfsRootZxart(appContext, http));
      composite.addSubroot(new VfsRootJoshw(appContext, http));
      composite.addSubroot(new VfsRootAmp(appContext, http));
      composite.addSubroot(new VfsRootAygor(appContext, http));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
