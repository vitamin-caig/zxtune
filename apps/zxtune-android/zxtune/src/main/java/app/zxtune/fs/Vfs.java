/**
 * @file
 * @brief VFS entry point
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

import app.zxtune.MainApplication;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.cache.CacheFactory;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.DownloadCommand;
import app.zxtune.fs.http.HttpObject;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.io.Io;

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
  public static ByteBuffer read(@NonNull final VfsFile file) throws IOException {
    final Uri uri = file.getUri();
    {
      final Object asFile = file.getExtension(VfsExtensions.FILE);
      if (asFile instanceof File) {
        return Io.readFrom((File) asFile);
      }
    }
    return new CommandExecutor(uri.getScheme()).executeDownloadCommand(new DownloadCommand() {
      @NonNull
      @Override
      public File getCache() throws IOException {
        final Object cache = file.getExtension(VfsExtensions.CACHE);
        if (cache instanceof File) {
          final File res = (File) cache;
          if (createParentDirFor(res)) {
            return res;
          }
        }
        throw new IOException("Failed to get cache for " + uri);
      }

      @NonNull
      @Override
      public HttpObject getRemote() throws IOException {
        final Object uris = file.getExtension(VfsExtensions.DOWNLOAD_URIS);
        if (uris instanceof Uri[]) {
          return Holder.INSTANCE.network.getObject((Uri[]) uris);
        }
        throw new IOException("Failed to get download uris for " + uri);
      }
    });
  }

  private static boolean createParentDirFor(@NonNull File file) {
    final File parent = file.getParentFile();
    return parent.isDirectory() || (parent.mkdirs() && parent.isDirectory());
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
