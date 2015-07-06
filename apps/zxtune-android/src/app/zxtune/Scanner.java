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

import java.io.IOException;
import java.io.InvalidObjectException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

import android.net.Uri;
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

  public interface Callback {
    
    void onModule(Identifier id, ZXTune.Module module);
    void onIOError(IOException e);
  }
  
  private final HashMap<Uri, VfsFile> archivesCache;
  
  public Scanner() {
    this.archivesCache = new HashMap<Uri, VfsFile>();
  }
  
  public final void analyze(Uri[] uris, Callback cb) {
    for (Uri uri : uris) {
      analyzeUri(uri, cb);
    }
  }
  
  public final void analyzeUri(Uri uri, Callback cb) {
    try {
      final Identifier id = new Identifier(uri);
      if (id.getSubpath().isEmpty()) {
        analyzeRealObject(uri, cb);
      } else {
        analyzeArchiveObject(id, cb);
      }
    } catch (IOException e) {
      cb.onIOError(e);
    }
  }
  
  private void analyzeRealObject(Uri uri, Callback cb) throws IOException {
    final VfsObject obj = Vfs.resolve(uri);
    analyzeObject(uri, obj, cb);
  }
  
  private void analyzeObject(Uri uri, VfsObject obj, Callback cb) throws IOException {
    if (obj instanceof VfsDir) {
      analyzeDir((VfsDir) obj, cb);
    } else if (obj instanceof VfsFile) {
      analyzeFile((VfsFile) obj, cb);
    } else {
      throw new IOException("Failed to resolve " + uri);
    }
  }
  
  private void analyzeDir(VfsDir directory, Callback cb) throws IOException {
    //analyze depth first
    final ArrayList<VfsDir> dirs = new ArrayList<VfsDir>();
    final ArrayList<VfsFile> files = new ArrayList<VfsFile>();
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
  
  private void analyzeFile(VfsFile file, Callback cb) throws IOException {
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
    } catch (IOException e) {
      cb.onIOError(e);
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
      final Uri uri = Uri.parse(dir.resolve(location).toString());
      analyzeUri(uri, cb);
    }
  }
      
  private static boolean isWindowsPath(String path) {
    return path.length() > 2 && path.charAt(1) == ':';
  }
  
  private static void analyzeRealFile(VfsFile file, final Callback cb) throws IOException {
    final Uri uri = file.getUri();
    final ByteBuffer content = file.getContent();
    ZXTune.detectModules(content, new ZXTune.ModuleDetectCallback() {
      
      @Override
      public void onModule(String subpath, Module obj) {
        try {
          cb.onModule(new Identifier(uri, subpath), obj);
        } catch (Error e) {
          obj.release();
          throw e;
        }
      }
    });
  }
  
  private void analyzeArchiveObject(Identifier id, Callback cb) throws IOException {
    try {
      //try to avoid VfsArchive
      analyzeArchiveFile(id, cb);
    } catch (IOException e) {
      final Uri uri = id.getFullLocation();
      final VfsObject obj = VfsArchive.resolve(uri);
      analyzeObject(uri, obj, cb);
    }
  }
  
  
  private void analyzeArchiveFile(Identifier id, Callback cb) throws IOException {
    final VfsFile archive = openArchive(id.getDataLocation());
    final ZXTune.Module module = openModule(archive, id.getSubpath());
    try {
      cb.onModule(id, module);
    } catch (Error e) {
      module.release();
      throw e;
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
  
  private static ZXTune.Module openModule(VfsFile file, String subpath) throws IOException {
    try {
      final ByteBuffer content = file.getContent();
      return ZXTune.loadModule(content, subpath);
    } catch (InvalidObjectException e) {
      throw new IOException("No module " + subpath + " in " + file.getUri());
    }
  }
}
