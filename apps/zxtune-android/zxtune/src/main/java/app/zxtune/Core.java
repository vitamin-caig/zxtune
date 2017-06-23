package app.zxtune;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Locale;

import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

public class Core {
  private final static String TAG = Core.class.getName();

  public static ZXTune.Module loadModule(VfsFile file, String subpath) throws Exception {
    final ByteBuffer content = file.getContent();
    Analytics.setFile(file.getUri(), subpath, content.limit());
    final ZXTune.Module obj = ZXTune.loadModule(content, subpath);
    final String[] files = obj.getAdditionalFiles();
    if (files == null || files.length == 0) {
      return obj;
    } else if (subpath.isEmpty()) {
      Log.d(TAG, "Resolve additional files for opened %s", file.getUri());
      return new Resolver(file).resolve(obj, files);
    } else {
      //was not resolved by ZXTune library core
      throw new Exception(String.format(Locale.US, "Unresolved additional files '%s'", files.toString()));
    }
  }

  public static void detectModules(VfsFile file, ZXTune.ModuleDetectCallback callback) throws Exception {
    final ByteBuffer content = file.getContent();
    Analytics.setFile(file.getUri(), "*", content.limit());
    final ModuleDetectCallbackAdapter adapter = new ModuleDetectCallbackAdapter(file, callback);
    ZXTune.detectModules(content, adapter);
    if (0 == adapter.getDetectedModulesCount()) {
      Analytics.sendNoTracksFoundEvent(file.getUri());
    }
  }

  private static class ModuleDetectCallbackAdapter implements ZXTune.ModuleDetectCallback {

    private final VfsFile location;
    private final ZXTune.ModuleDetectCallback delegate;
    private Resolver resolver;
    private int modulesCount = 0;

    ModuleDetectCallbackAdapter(VfsFile location, ZXTune.ModuleDetectCallback delegate) {
      this.location = location;
      this.delegate = delegate;
    }

    final int getDetectedModulesCount() {
      return modulesCount;
    }

    @Override
    public void onModule(String subpath, ZXTune.Module obj) throws Exception {
      ++modulesCount;
      try {
        final String[] files = obj.getAdditionalFiles();
        if (files == null || files.length == 0) {
          delegate.onModule(subpath, obj);
        } else if (subpath.isEmpty()) {
          Log.d(TAG, "Resolve additional files for detected %s", location.getUri());
          delegate.onModule(subpath, resolve(obj, files));
        } else {
          //was not resolved by core
          Log.d(TAG, "Unresolved additional files '%s'", files.toString());
        }
      } catch (IOException e) {
        Log.w(TAG, e, "Skip module at %s in %s", subpath, location.getUri());
      }
    }

    private ZXTune.Module resolve(ZXTune.Module obj, String[] files) throws Exception {
      return getResolver().resolve(obj, files);
    }

    private Resolver getResolver() {
      if (resolver == null) {
        resolver = new Resolver(location);
      }
      return resolver;
    }
  }

  private static class Resolver {

    private VfsDir parent;
    private HashMap<String, VfsFile> files = new HashMap<String, VfsFile>();
    private HashMap<String, VfsDir> dirs = new HashMap<String, VfsDir>();

    Resolver(VfsFile content) {
      final VfsObject parent = content.getParent();
      if (parent instanceof VfsDir) {
        this.parent = (VfsDir) parent;
      }
    }

    final ZXTune.Module resolve(ZXTune.Module module, String[] files) throws Exception {
      while (files != null && 0 != files.length) {
        resolveIteration(module, files);
        String[] newFiles = module.getAdditionalFiles();
        if (files.equals(newFiles)) {
          throw new Exception("Nothing resolved");
        }
        files = newFiles;
      }
      return module;
    }

    private void resolveIteration(ZXTune.Module module, String[] files) throws Exception {
      for (String name : files) {
        Log.d(TAG, "Resolve %s", name);
        module.resolveAdditionalFile(name, getFileContent(name));
      }
    }

    private ByteBuffer getFileContent(String name) throws IOException {
      final VfsFile file = findFile(name);
      if (file == null) {
        throw new IOException(String.format(Locale.US, "Failed to find additional file '%s'", name));
      }
      return file.getContent();
    }

    @Nullable
    private VfsFile findFile(String name) throws IOException {
      if (parent != null) {
        final Uri fileUri = parent.getUri().buildUpon().appendPath(name).build();
        Log.d(TAG, "Try to find '%s' as '%s'", name, fileUri);
        final VfsObject res = Vfs.resolve(fileUri);
        if (res instanceof VfsFile) {
          return (VfsFile) res;
        }
        preloadDir(parent, "");
        parent = null;
      }
      return findPreloadedFile(name);
    }

    @Nullable
    private VfsFile findPreloadedFile(String name) throws IOException {
      if (!files.containsKey(name)) {
        final int lastSeparator = name.lastIndexOf('/');
        if (lastSeparator != -1) {
          preloadDir(name.substring(0, lastSeparator - 1));
        }
      }
      return files.get(name);
    }

    private void preloadDir(String path) throws IOException {
      if (!dirs.containsKey(path)) {
        final int lastSeparator = path.lastIndexOf('/');
        if (lastSeparator != -1) {
          preloadDir(path.substring(0, lastSeparator - 1));
        }
      }
      final VfsDir dir = dirs.get(path);
      if (dir == null) {
        throw new IOException(String.format(Locale.US, "Failed to find additional file directory '%s'", path));
      }
      preloadDir(dir, path + "/");
    }

    private void preloadDir(VfsDir dir, final String relPath) throws IOException {
      Log.d(TAG, "Preload content of %s as '%s'", dir.getUri(), relPath);
      dir.enumerate(new VfsDir.Visitor() {

        @Override
        public void onItemsCount(int count) {
        }

        @Override
        public void onDir(VfsDir dir) {
          final String name = relPath + dir.getName();
          dirs.put(name, dir);
          Log.d(TAG, "Add dir %s (%s)", name, dir.getUri());
        }

        @Override
        public void onFile(VfsFile file) {
          final String name = relPath + file.getName();
          files.put(name, file);
          Log.d(TAG, "Add file %s (%s)", name, file.getUri());
        }
      });
    }
  }
}
