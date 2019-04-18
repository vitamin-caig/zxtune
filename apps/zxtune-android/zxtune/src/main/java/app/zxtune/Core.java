package app.zxtune;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;

import app.zxtune.core.Module;
import app.zxtune.core.ModuleDetectCallback;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

public class Core {
  private static final String TAG = Core.class.getName();

  public static Module loadModule(VfsFile file, String subpath) throws Exception {
    final ByteBuffer content = file.getContent();
    final Analytics.JniLog log = new Analytics.JniLog(file.getUri(), subpath, content.limit());
    log.action("loadModule begin");
    final Module obj = ZXTune.loadModule(makeDirectBuffer(content), subpath);
    log.action("loadModule end");
    final String[] files = obj.getAdditionalFiles();
    if (files == null || files.length == 0) {
      return obj;
    } else if (subpath.isEmpty()) {
      Log.d(TAG, "Resolve additional files for opened %s", file.getUri());
      return new Resolver(file, log).resolve(obj, files);
    } else {
      //was not resolved by ZXTune library core
      throw new ResolvingException(String.format(Locale.US, "Unresolved additional files '%s'", Arrays.toString(files)));
    }
  }

  public static void detectModules(VfsFile file, ModuleDetectCallback callback) throws Exception {
    final ByteBuffer content = file.getContent();
    final Analytics.JniLog log = new Analytics.JniLog(file.getUri(), "*", content.limit());
    final ModuleDetectCallbackAdapter adapter = new ModuleDetectCallbackAdapter(file, callback, log);
    log.action("detectModules begin");
    ZXTune.detectModules(makeDirectBuffer(content), adapter);
    log.action("detectModules end");
    if (0 == adapter.getDetectedModulesCount()) {
      Analytics.sendNoTracksFoundEvent(file.getUri());
    }
  }

  private static ByteBuffer makeDirectBuffer(ByteBuffer content) throws Exception {
    if (content.position() != 0) {
      throw new Exception("Input data should have zero position");
    }
    if (content.isDirect()) {
      return content;
    } else {
      final ByteBuffer direct = ByteBuffer.allocateDirect(content.limit());
      direct.put(content);
      direct.position(0);
      content.position(0);
      return direct;
    }
  }

  private static class ModuleDetectCallbackAdapter implements ModuleDetectCallback {

    private final VfsFile location;
    private final ModuleDetectCallback delegate;
    private final Analytics.JniLog log;
    private Resolver resolver;
    private int modulesCount = 0;

    ModuleDetectCallbackAdapter(VfsFile location, ModuleDetectCallback delegate, Analytics.JniLog log) {
      this.location = location;
      this.delegate = delegate;
      this.log = log;
    }

    final int getDetectedModulesCount() {
      return modulesCount;
    }

    @Override
    public void onModule(String subpath, Module obj) throws Exception {
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
          Log.d(TAG, "Unresolved additional files '%s'", Arrays.toString(files));
        }
      } catch (IOException|ResolvingException e) {
        Log.w(TAG, e, "Skip module at %s in %s", subpath, location.getUri());
      }
    }

    private Module resolve(Module obj, String[] files) throws Exception {
      return getResolver().resolve(obj, files);
    }

    private Resolver getResolver() {
      if (resolver == null) {
        resolver = new Resolver(location, log);
      }
      return resolver;
    }
  }

  private static class Resolver {

    private VfsDir parent;
    private final Analytics.JniLog log;
    private final HashMap<String, VfsFile> files = new HashMap<>();
    private final HashMap<String, VfsDir> dirs = new HashMap<>();

    Resolver(VfsFile content, Analytics.JniLog log) {
      final VfsObject parent = content.getParent();
      if (parent instanceof VfsDir) {
        this.parent = (VfsDir) parent;
      }
      this.log = log;
    }

    final Module resolve(Module module, String[] files) throws ResolvingException {
      while (files != null && 0 != files.length) {
        final String[] newFiles = resolveIteration(module, files);
        if (Arrays.equals(files, newFiles)) {
          throw new ResolvingException("Nothing resolved");
        }
        files = newFiles;
      }
      return module;
    }

    private String[] resolveIteration(Module module, String[] files) throws ResolvingException {
      try {
        for (String name : files) {
          final ByteBuffer content = getFileContent(name);
          log.action("resolveAdditionalFile " + name);
          module.resolveAdditionalFile(name, makeDirectBuffer(content));
          log.action("resolveAdditionalFile end");
        }
        return module.getAdditionalFiles();
      } catch (Exception e) {
        throw new ResolvingException("Failed to resolve additional files", e);
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

class ResolvingException extends Exception {
  private static final long serialVersionUID = 1L;

  ResolvingException(String msg) {
    super(msg);
  }

  ResolvingException(String msg, Throwable e) {
    super(msg, e);
  }
}