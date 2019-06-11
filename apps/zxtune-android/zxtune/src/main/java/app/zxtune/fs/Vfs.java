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
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.cache.CacheFactory;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;

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
      final HttpProvider http = HttpProviderFactory.createProvider(appContext);
      final CacheDir cache = CacheFactory.create(appContext);
      composite.addSubroot(new VfsRootLocal(appContext));
      composite.addSubroot(new VfsRootZxtunes(appContext, http, cache));
      composite.addSubroot(new VfsRootPlaylists(appContext));
      composite.addSubroot(new VfsRootModland(appContext, http, cache));
      composite.addSubroot(new VfsRootHvsc(appContext, http, cache));
      composite.addSubroot(new VfsRootZxart(appContext, http, cache));
      composite.addSubroot(new VfsRootJoshw(appContext, http, cache));
      composite.addSubroot(new VfsRootAmp(appContext, http, cache));
      composite.addSubroot(new VfsRootAygor(appContext, http, cache));
      composite.addSubroot(new VfsRootModarchive(appContext, http, cache));
      composite.addSubroot(new VfsRootAsma(appContext, http, cache));
      composite.addSubroot(new VfsRootAminet(appContext, http, cache));
      rootSingleton = composite;
    }
    return rootSingleton;
  }
}
