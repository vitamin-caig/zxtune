/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.net.Uri;

import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

import app.zxtune.ZXTune.Module;
import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.XspfIterator;

public final class Scanner {

  private static final String TAG = Scanner.class.getName();

  public interface Callback {
    
    void onModule(Identifier id, ZXTune.Module module);
    void onError(Exception e);
  }
  
  private final HashMap<Uri, VfsFile> archivesCache;
  
  public Scanner() {
    this.archivesCache = new HashMap<>();
  }
  
  public final void analyze(Uri[] uris, Callback cb) {
    for (Uri uri : uris) {
      analyzeIdentifier(new Identifier(uri), cb);
    }
  }
  
  public final void analyzeIdentifier(Identifier id, Callback cb) {
    try {
      if (id.getSubpath().isEmpty()) {
        analyzeRealObject(id.getDataLocation(), cb);
      } else {
        analyzeArchiveObject(id, cb);
      }
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
      analyzeDir((VfsDir) obj, cb);
    } else if (obj instanceof VfsFile) {
      analyzeFile((VfsFile) obj, cb);
    }
  }
  
  private void analyzeDir(VfsDir directory, Callback cb) throws Exception {
    //analyze depth first
    final ArrayList<VfsDir> dirs = new ArrayList<>();
    final ArrayList<VfsFile> files = new ArrayList<>();
    directory.enumerate(new VfsDir.Visitor() {
      
      @Override
      public void onItemsCount(int count) {
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
    final Comparator<VfsObject> comparator = directory instanceof Comparator<?>
      ? (Comparator<VfsObject>) directory
      : DefaultComparator.instance();
    Collections.sort(dirs, comparator);
    Collections.sort(files, comparator);
    
    for (VfsDir dir : dirs) {
      analyzeDir(dir, cb);
    }
    for (VfsFile file : files) {
      analyzeFile(file, cb);
    }
  }
  
  private void analyzeFile(VfsFile file, Callback cb) throws Exception {
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
        try {
          cb.onModule(new Identifier(uri, subpath), obj);
        } catch (Exception e) {
          obj.release();
          Log.w(TAG, e, "Failed analyzeRealFile.onModule");
          //do not rethrow error - it comes from particular module, not from callback
          //throw e;
        }
      }
    });
  }
  
  private void analyzeArchiveObject(Identifier id, Callback cb) throws Exception {
    if (!analyzeArchiveFile(id, cb)) {
      final Uri uri = id.getFullLocation();
      final VfsObject obj = VfsArchive.resolve(uri);
      if (obj instanceof VfsDir) {
        analyzeDir((VfsDir) obj, cb);
      }
      //expired archive file
    }
  }
  
  private boolean analyzeArchiveFile(Identifier id, Callback cb) throws Exception {
    final VfsFile archive = openArchive(id.getDataLocation());
    final ZXTune.Module module;
    try {
      module = Core.loadModule(archive, id.getSubpath());
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to analyzer archive file");
      return false;
    }
    try {
      cb.onModule(id, module);
    } catch (Exception e) {
      module.release();
      throw e;
    }
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
}
