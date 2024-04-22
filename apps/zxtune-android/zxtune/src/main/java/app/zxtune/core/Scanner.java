package app.zxtune.core;

import android.net.Uri;
import android.util.LruCache;

import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.xspf.XspfIterator;
import app.zxtune.utils.StubProgressCallback;

public final class Scanner {

  public interface Callback {

    void onModule(Identifier id, Module module);

    void onError(Identifier id, Exception e);
  }

  private static final int CACHE_ENTRIES = 30;
  private final LruCache<Uri, VfsFile> archivesCache;

  private Scanner() {
    this.archivesCache = new LruCache<Uri, VfsFile>(CACHE_ENTRIES) {
      @Override
      protected int sizeOf(Uri key, VfsFile value) {
        return 1;
      }
    };
  }

  public static void analyzeIdentifier(Identifier id, Callback cb) {
    try {
      if (id.getSubPath().isEmpty()) {
        Holder.INSTANCE.analyzeRealObject(id, cb);
      } else {
        Holder.INSTANCE.analyzeArchiveObject(id, cb);
      }
    } catch (Exception e) {
      cb.onError(id, e);
    }
  }

  public static void analyzeFile(VfsFile file, Callback cb) {
    Holder.INSTANCE.analyzeFileObject(file, cb);
  }

  private void analyzeRealObject(Identifier id, Callback cb) throws Exception {
    final VfsObject obj = Vfs.resolve(id.getDataLocation());
    if (obj instanceof VfsDir) {
      analyzeDirObject((VfsDir) obj, cb);
    } else if (obj instanceof VfsFile) {
      analyzeFileObject((VfsFile) obj, cb);
    }
  }

  private void analyzeDirObject(VfsDir directory, Callback cb) {
    //analyze depth first
    final ArrayList<VfsDir> dirs = new ArrayList<>();
    final ArrayList<VfsFile> files = new ArrayList<>();
    try {
      directory.enumerate(new VfsDir.Visitor() {

        @Override
        public void onItemsCount(int count) {
          files.ensureCapacity(count);
        }

        @Override
        public void onFile(VfsFile file) {
          files.add(file);
        }

        @Override
        public void onDir(VfsDir dir) {
          dirs.add(dir);
        }
      });
    } catch (Exception e) {
      cb.onError(new Identifier(directory.getUri()), e);
      return;
    }
    final Object extension = directory.getExtension(VfsExtensions.COMPARATOR);
    final Comparator<VfsObject> comparator = extension instanceof Comparator<?>
                                                 ? (Comparator<VfsObject>) extension
                                                 : DefaultComparator.instance();
    Collections.sort(dirs, comparator);
    Collections.sort(files, comparator);

    for (VfsDir dir : dirs) {
      analyzeDirObject(dir, cb);
    }
    for (VfsFile file : files) {
      analyzeFileObject(file, cb);
    }
  }

  private void analyzeFileObject(VfsFile file, Callback cb) {
    final Identifier id = new Identifier(file.getUri());
    try {
      //may be called from recursion, so additionally check for archived objects
      if (!id.getSubPath().isEmpty()) {
        analyzeArchiveObject(id, cb);
      } else if (!analyzePlaylistFile(file, cb)) {
        analyzeRealFile(file, cb);
      }
    } catch (Exception e) {
      cb.onError(id, e);
    }
  }

  //TODO: reuse VfsPlaylistDir
  private boolean analyzePlaylistFile(VfsFile file, Callback cb) throws IOException {
    final Uri uri = file.getUri();
    final String filename = uri.getLastPathSegment();
    if (filename == null) {
      return false;
    } else if (filename.endsWith(".ayl")) {
      analyzePlaylist(file.getUri(), AylIterator.create(Vfs.openStream(file)), cb);
      return true;
    } else if (filename.endsWith(".xspf") || "playlists".equals(uri.getScheme())) {
      analyzePlaylist(file.getUri(), XspfIterator.create(Vfs.openStream(file)), cb);
      return true;
    } else {
      return false;
    }
  }

  private void analyzePlaylist(Uri fileUri, ReferencesIterator iter, Callback cb) {
    final URI dir = URI.create(fileUri.toString()).resolve(".");
    while (iter.next()) {
      final String location = iter.getItem().location;
      if (isWindowsPath(location)) {
        continue;//windows paths are not supported
      }
      final Identifier id = Identifier.parse(dir.resolve(location).toString());
      analyzeIdentifier(id, cb);
    }
  }

  private static boolean isWindowsPath(String path) {
    return path.length() > 2 && path.charAt(1) == ':';
  }

  private static void analyzeRealFile(VfsFile file, final Callback cb) throws Exception {
    Core.detectModules(file, cb::onModule);
  }

  private void analyzeArchiveObject(Identifier id, Callback cb) throws Exception {
    try {
      final VfsFile archive = openArchive(id.getDataLocation());
      final Module module = Core.loadModule(archive, id.getSubPath());
      cb.onModule(id, module);
    } catch (ResolvingException e) {
      final Uri uri = id.getFullLocation();
      final VfsObject obj = VfsArchive.resolveForced(uri, StubProgressCallback.instance());
      if (obj instanceof VfsDir) {
        analyzeDirObject((VfsDir) obj, cb);
      } else {
        // Archived folder with expired cache
        throw e;
      }
    }
  }

  private VfsFile openArchive(Uri uri) throws IOException {
    final VfsFile arch = archivesCache.get(uri);
    if (arch != null) {
      return arch;
    }
    final VfsObject obj = Vfs.resolve(uri);
    if (obj instanceof VfsFile) {
      archivesCache.put(uri, (VfsFile) obj);
      return (VfsFile) obj;
    } else {
      throw new IOException("Failed to resolve archive " + uri);
    }
  }

  private static class Holder {
    static final Scanner INSTANCE = new Scanner();
  }
}
