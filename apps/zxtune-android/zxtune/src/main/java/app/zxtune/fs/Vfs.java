/**
 * @file
 * @brief VFS entry point
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.Nullable;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Iterator;

import app.zxtune.Features;
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
import app.zxtune.utils.ProgressCallback;

public final class Vfs {

  private final MultisourceHttpProvider network;
  private final CacheDir cache;
  private final VfsRoot root;

  private Vfs() {
    final Context appContext = MainApplication.getGlobalContext();
    final HttpProvider http = HttpProviderFactory.createProvider(appContext);
    network = new MultisourceHttpProvider(http);
    cache = CacheFactory.create(appContext);
    root = createRoot(appContext);
  }

  public static VfsDir getRoot() {
    return Holder.INSTANCE.root;
  }

  public static VfsObject resolve(Uri uri) throws IOException {
    final VfsObject res = Holder.INSTANCE.root.resolve(uri);
    if (res != null) {
      return res;
    } else {
      throw new IOException("Failed to resolve " + uri);
    }
  }

  @SuppressWarnings("unchecked")
  public static InputStream openStream(VfsFile file) throws IOException {
    final InputStream asStream = VfsExtensionsKt.getInputStream(file);
    if (asStream != null) {
      return asStream;
    }
    final File asFile = VfsExtensionsKt.getFile(file);
    if (asFile != null) {
      return new FileInputStream(asFile);
    }
    final FileDescriptor asDescriptor = VfsExtensionsKt.getFileDescriptor(file);
    if (asDescriptor != null) {
      return new FileInputStream(asDescriptor);
    }
    if (file.getExtension(VfsExtensions.CACHE_PATH) == null) {
      final Object uris = file.getExtension(VfsExtensions.DOWNLOAD_URIS);
      if (uris instanceof Uri[]) {
        return new BufferedInputStream(Holder.INSTANCE.network.getInputStream((Uri[]) uris));
      } else if (uris instanceof Iterator) {
        return new BufferedInputStream(Holder.INSTANCE.network.getInputStream((Iterator<Uri>) uris));
      }
    }
    return Io.createByteBufferInputStream(download(file, null));
  }

  public static ByteBuffer read(final VfsFile file) throws IOException {
    return read(file, null);
  }

  public static ByteBuffer read(final VfsFile file, @Nullable ProgressCallback progress) throws IOException {
    final File asFile = VfsExtensionsKt.getFile(file);
    if (asFile != null) {
      return Io.readFrom(asFile);
    }
    final FileDescriptor asDescriptor = VfsExtensionsKt.getFileDescriptor(file);
    if (asDescriptor != null) {
      return Io.readFrom(new FileInputStream(asDescriptor));
    }
    return download(file, progress);
  }

  private static ByteBuffer download(final VfsFile file, @Nullable ProgressCallback progress) throws IOException {
    final Uri uri = file.getUri();
    return new CommandExecutor(uri.getScheme()).executeDownloadCommand(new DownloadCommand() {
      @Override
      public File getCache() throws IOException {
        final File res = Vfs.getCache(file);
        if (res != null && (res.isFile() || createParentDirFor(res))) {
          return res;
        }
        throw new IOException("Failed to get cache for " + uri);
      }

      @Override
      @SuppressWarnings("unchecked")
      public HttpObject getRemote() throws IOException {
        final Object uris = file.getExtension(VfsExtensions.DOWNLOAD_URIS);
        if (uris instanceof Uri[]) {
          return Holder.INSTANCE.network.getObject((Uri[]) uris);
        } else if (uris instanceof Iterator) {
          return Holder.INSTANCE.network.getObject((Iterator<Uri>) uris);
        }
        throw new IOException("Failed to get download uris for " + uri);
      }
    }, progress);
  }

  @Nullable
  public static File getCache(VfsFile obj) {
    final String id = obj.getUri().getScheme();
    if (TextUtils.isEmpty(id)) {
      return null;
    }
    final String path = (String) obj.getExtension(VfsExtensions.CACHE_PATH);
    if (path == null) {
      return null;
    }
    final String compatId = getCacheCompatId(id);
    return Holder.INSTANCE.cache.find(id + "/" + path, compatId + "/" + path);
  }

  private static String getCacheCompatId(String id) {
    switch (id) {
      case "amp":
        return "amp.dascene.net";
      case "modarchive":
        return "modarchive.org";
      case "modland":
        return "ftp.modland.com";
      case "zxart":
        return "www.zxart.ee";
      case "zxtunes":
        return "www.zxtunes.com";
      default:
        return id;
    }
  }

  private static boolean createParentDirFor(File file) {
    final File parent = file.getParentFile();
    return parent.isDirectory() || (parent.mkdirs() && parent.isDirectory());
  }

  private VfsRoot createRoot(Context appContext) {
    final VfsRootComposite composite = new VfsRootComposite(null);
    if (Features.StorageAccessFramework.isEnabled()) {
      composite.addSubroot(new VfsRootLocalStorageAccessFramework(appContext));
    } else {
      composite.addSubroot(new VfsRootLocal(appContext));
    }
    composite.addSubroot(new VfsRootNetwork(appContext, network));
    composite.addSubroot(new VfsRootPlaylists(appContext));
    composite.addSubroot(new VfsRootRadio(appContext));
    return composite;
  }

  //onDemand holder idiom
  private static class Holder {
    static final Vfs INSTANCE = new Vfs();
  }
}
