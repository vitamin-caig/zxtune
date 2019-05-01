package app.zxtune.core;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.core.jni.JniModule;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;

public class Core {
  private static final String TAG = Core.class.getName();

  @NonNull
  public static Module loadModule(@NonNull VfsFile file, @NonNull String subpath) throws IOException, ResolvingException {
    final ByteBuffer content = file.getContent();
    final Analytics.JniLog log = new Analytics.JniLog(file.getUri(), subpath, content.limit());
    log.action("loadModule begin");
    final Module obj = JniModule.load(makeDirectBuffer(content), subpath);
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

  public static void detectModules(@NonNull VfsFile file, @NonNull ModuleDetectCallback callback) throws IOException {
    final ByteBuffer content = file.getContent();
    final Analytics.JniLog log = new Analytics.JniLog(file.getUri(), "*", content.limit());
    final ModuleDetectCallbackAdapter adapter = new ModuleDetectCallbackAdapter(file, callback, log);
    log.action("detectModules begin");
    JniModule.detect(makeDirectBuffer(content), adapter);
    log.action("detectModules end");
    if (0 == adapter.getDetectedModulesCount()) {
      Analytics.sendNoTracksFoundEvent(file.getUri());
    }
  }

  @NonNull
  private static ByteBuffer makeDirectBuffer(@NonNull ByteBuffer content) {
    if (content.position() != 0) {
      throw new IllegalArgumentException("Input data should have zero position");
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
    public void onModule(@NonNull String subpath, @NonNull Module obj) {
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
      } catch (ResolvingException e) {
        Log.w(TAG, e, "Skip module at %s in %s", subpath, location.getUri());
      }
    }

    @NonNull
    private Module resolve(@NonNull Module obj, String[] files) throws ResolvingException {
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

    @Nullable
    private VfsDir parent;
    private final Analytics.JniLog log;
    private final HashMap<String, VfsFile> files = new HashMap<>();
    private final HashMap<String, VfsDir> dirs = new HashMap<>();

    Resolver(@NonNull VfsFile content, @NonNull Analytics.JniLog log) {
      final VfsObject parent = content.getParent();
      if (parent instanceof VfsDir) {
        this.parent = (VfsDir) parent;
      }
      this.log = log;
    }

    @NonNull
    final Module resolve(@NonNull Module module, @Nullable String[] files) throws ResolvingException {
      while (files != null && 0 != files.length) {
        final String[] newFiles = resolveIteration(module, files);
        if (Arrays.equals(files, newFiles)) {
          throw new ResolvingException("Nothing resolved");
        }
        files = newFiles;
      }
      return module;
    }

    @Nullable
    private String[] resolveIteration(@NonNull Module module, @NonNull String[] files) throws ResolvingException {
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

    private ByteBuffer getFileContent(@NonNull String name) throws IOException {
      final VfsFile file = findFile(name);
      if (file == null) {
        throw new IOException(String.format(Locale.US, "Failed to find additional file '%s'", name));
      }
      return file.getContent();
    }

    @Nullable
    private VfsFile findFile(@NonNull String name) throws IOException {
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
    private VfsFile findPreloadedFile(@NonNull String name) throws IOException {
      if (!files.containsKey(name)) {
        final int lastSeparator = name.lastIndexOf('/');
        if (lastSeparator != -1) {
          preloadDir(name.substring(0, lastSeparator - 1));
        }
      }
      return files.get(name);
    }

    private void preloadDir(@NonNull String path) throws IOException {
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

    private void preloadDir(@NonNull VfsDir dir, @NonNull final String relPath) throws IOException {
      Log.d(TAG, "Preload content of %s as '%s'", dir.getUri(), relPath);
      dir.enumerate(new VfsDir.Visitor() {

        @Override
        public void onItemsCount(int count) {
        }

        @Override
        public void onDir(@NonNull VfsDir dir) {
          final String name = relPath + dir.getName();
          dirs.put(name, dir);
          Log.d(TAG, "Add dir %s (%s)", name, dir.getUri());
        }

        @Override
        public void onFile(@NonNull VfsFile file) {
          final String name = relPath + file.getName();
          files.put(name, file);
          Log.d(TAG, "Add file %s (%s)", name, file.getUri());
        }
      });
    }
  }
}

