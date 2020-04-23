/**
 * @file
 * @brief VFS entry point
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.MainApplication;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.cache.CacheFactory;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;

public final class Vfs {

  private final MultisourceHttpProvider network;
  private final CacheDir cache;
  private final VfsRoot root;

  private Vfs() {
    final Context appContext = MainApplication.getInstance();
    final HttpProvider http = HttpProviderFactory.createProvider(appContext);
    network = new MultisourceHttpProvider(http);
    cache = CacheFactory.create(appContext);
    root = createRoot(appContext);
  }

  @NonNull
  public static VfsDir getRoot() {
    return Holder.INSTANCE.root;
  }

  @NonNull
  public static VfsObject resolve(Uri uri) throws IOException {
    final VfsObject res = Holder.INSTANCE.root.resolve(uri);
    if (res != null) {
      return res;
    } else {
      throw new IOException("Failed to resolve " + uri);
    }
  }

  @NonNull
  public static ByteBuffer read(@NonNull VfsFile file) throws IOException {
    return file.getContent();
  }

  private VfsRoot createRoot(Context appContext) {
    final VfsRootComposite composite = new VfsRootComposite(null);
    composite.addSubroot(new VfsRootLocal(appContext));
    composite.addSubroot(new VfsRootNetwork(appContext, network, cache));
    composite.addSubroot(new VfsRootPlaylists(appContext));
    composite.addSubroot(new VfsRootRadio(appContext));
    return composite;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Vfs INSTANCE = new Vfs();
  }
}
