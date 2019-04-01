/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.core;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.util.LruCache;
import app.zxtune.Core;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.ZXTune;
import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.XspfIterator;

import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

public final class Scanner {

  private static final String TAG = Scanner.class.getName();

  public interface Callback {

    void onModule(Identifier id, Module module) throws Exception;

    void onError(Exception e);
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

  public static void analyzeIdentifier(@NonNull Identifier id, @NonNull Callback cb) {
    try {
      if (id.getSubpath().isEmpty()) {
        Holder.INSTANCE.analyzeRealObject(id.getDataLocation(), cb);
      } else {
        Holder.INSTANCE.analyzeArchiveObject(id, cb);
      }
    } catch (Exception e) {
      cb.onError(e);
    }
  }

  public static void analyzeFile(@NonNull VfsFile file, @NonNull Callback cb) {
    try {
      Holder.INSTANCE.analyzeFileObject(file, cb);
    } catch (Exception e) {
      cb.onError(e);
    }
  }

  private void analyzeRealObject(Uri uri, Callback cb) throws Exception {
    final VfsObject obj = Vfs.resolve(uri);
    analyzeObject(obj, cb);
  }

  private void analyzeObject(VfsObject obj, Callback cb) throws Exception {
    if (obj instanceof VfsDir) {
      analyzeDirObject((VfsDir) obj, cb);
    } else if (obj instanceof VfsFile) {
      analyzeFileObject((VfsFile) obj, cb);
    }
  }

  private void analyzeDirObject(VfsDir directory, Callback cb) throws Exception {
    //analyze depth first
    final ArrayList<VfsDir> dirs = new ArrayList<>();
    final ArrayList<VfsFile> files = new ArrayList<>();
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

  private void analyzeFileObject(VfsFile file, Callback cb) throws Exception {
    //may be called from recursion, so additionally check for archived objects
    final Identifier id = new Identifier(file.getUri());
    if (!id.getSubpath().isEmpty()) {
      analyzeArchiveObject(id, cb);
    } else if (!analyzePlaylistFile(file, cb)) {
      analyzeRealFile(file, cb);
    }
  }

  private boolean analyzePlaylistFile(VfsFile file, Callback cb) {
    final String filename = file.getUri().getLastPathSegment();
    if (filename == null) {
      return false;
    }
    try {
      if (filename.endsWith(".ayl")) {
        analyzePlaylist(file.getUri(), AylIterator.create(file.getContent()), cb);
        return true;
      } else if (filename.endsWith(".xspf")) {
        analyzePlaylist(file.getUri(), XspfIterator.create(file.getContent()), cb);
        return true;
      }
    } catch (Exception e) {
      cb.onError(e);
    }
    return false;
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
    final Uri uri = file.getUri();
    Core.detectModules(file, new ZXTune.ModuleDetectCallback() {

      @Override
      public void onModule(String subpath, Module obj) throws Exception {
        cb.onModule(new Identifier(uri, subpath), obj);
      }
    });
  }

  private void analyzeArchiveObject(Identifier id, Callback cb) throws Exception {
    if (!analyzeArchiveFile(id, cb)) {
      final Uri uri = id.getFullLocation();
      final VfsObject obj = VfsArchive.resolve(uri);
      if (obj instanceof VfsDir) {
        analyzeDirObject((VfsDir) obj, cb);
      }
      //expired archive file
    }
  }

  private boolean analyzeArchiveFile(Identifier id, Callback cb) throws Exception {
    final VfsFile archive = openArchive(id.getDataLocation());
    final Module module;
    try {
      module = Core.loadModule(archive, id.getSubpath());
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to analyzer archive file");
      return false;
    }
    cb.onModule(id, module);
    return true;
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
    public static final Scanner INSTANCE = new Scanner();
  }
}
