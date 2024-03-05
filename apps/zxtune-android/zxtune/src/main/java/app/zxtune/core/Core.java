package app.zxtune.core;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.analytics.Analytics;
import app.zxtune.core.jni.Api;
import app.zxtune.core.jni.DetectCallback;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.utils.ProgressCallback;

public class Core {
  private static final String TAG = Core.class.getName();

  static Module loadModule(VfsFile file, String subpath) throws IOException, ResolvingException {
    return loadModule(file, subpath, null);
  }

  @SuppressWarnings("SameParameterValue")
  private static Module loadModule(VfsFile file, String subpath,
                                   @Nullable ProgressCallback progress) throws IOException,
      ResolvingException {
    final ByteBuffer content = Vfs.read(file);
    Analytics.setNativeCallTags(file.getUri(), subpath, content.limit());
    final Module obj = Api.instance().loadModule(makeDirectBuffer(content), subpath);
    final String[] files = obj.getAdditionalFiles();
    if (files == null || files.length == 0) {
      return obj;
    } else if (subpath.isEmpty()) {
      Log.d(TAG, "Resolve additional files for opened %s", file.getUri());
      return new Resolver(file, progress).resolve(obj, files);
    } else {
      //was not resolved by ZXTune library core
      throw new ResolvingException(String.format(Locale.US, "Unresolved additional files '%s'", Arrays.toString(files)));
    }
  }

  static void detectModules(VfsFile file, ModuleDetectCallback callback) throws IOException {
    detectModules(file, callback, null);
  }

  public static void detectModules(VfsFile file,
                                   ModuleDetectCallback callback,
                                   @Nullable ProgressCallback progress) throws IOException {
    final ByteBuffer content = Vfs.read(file, progress);
    final DetectCallbackAdapter adapter = new DetectCallbackAdapter(file, callback,
        progress);
    Analytics.setNativeCallTags(file.getUri(), "*", content.limit());
    Api.instance().detectModules(makeDirectBuffer(content), adapter, progress);
    if (0 == adapter.getDetectedModulesCount()) {
      Analytics.sendNoTracksFoundEvent(file.getUri());
    }
  }

  private static ByteBuffer makeDirectBuffer(ByteBuffer content) {
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

  private static class DetectCallbackAdapter implements DetectCallback {

    private final VfsFile source;
    private final Uri location;
    private final ModuleDetectCallback delegate;
    @Nullable
    private final ProgressCallback progress;
    @Nullable
    private Resolver resolver;
    private int modulesCount = 0;

    DetectCallbackAdapter(VfsFile source, ModuleDetectCallback delegate,
                          @Nullable ProgressCallback progress) {
      this.source = source;
      this.location = source.getUri();
      this.delegate = delegate;
      this.progress = progress;
    }

    final int getDetectedModulesCount() {
      return modulesCount;
    }

    @Override
    public void onModule(String subPath, Module obj) {
      ++modulesCount;
      try {
        final Identifier id = new Identifier(location, subPath);
        final String[] files = obj.getAdditionalFiles();
        if (files == null || files.length == 0) {
          delegate.onModule(id, obj);
        } else if (subPath.isEmpty()) {
          Log.d(TAG, "Resolve additional files for detected %s", location);
          delegate.onModule(id, resolve(obj, files));
        } else {
          //was not resolved by core
          Log.d(TAG, "Unresolved additional files '%s'", Arrays.toString(files));
        }
      } catch (ResolvingException e) {
        Log.w(TAG, e, "Skip module at %s in %s", subPath, location);
      }
    }

    @Override
    public void onPicture(String subPath, byte[] data) {
      Log.d(TAG, "Picture at %s in %d bytes", subPath, data.length);
    }

    private Module resolve(Module obj, String[] files) throws ResolvingException {
      return getResolver().resolve(obj, files);
    }

    private Resolver getResolver() {
      if (resolver == null) {
        resolver = new Resolver(source, progress);
      }
      return resolver;
    }
  }

  private static class Resolver {

    @Nullable
    private VfsDir parent;
    @Nullable
    private final ProgressCallback progress;
    private final HashMap<String, VfsFile> files = new HashMap<>();
    private final HashMap<String, VfsDir> dirs = new HashMap<>();

    Resolver(VfsFile content, @Nullable ProgressCallback progress) {
      final VfsObject parent = content.getParent();
      if (parent instanceof VfsDir) {
        this.parent = (VfsDir) parent;
      }
      this.progress = progress;
    }

    final Module resolve(Module module, @Nullable String[] unknownFiles) throws ResolvingException {
      String[] files = unknownFiles;
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
    private String[] resolveIteration(Module module, String[] files) throws ResolvingException {
      try {
        for (String name : files) {
          final ByteBuffer content = getFileContent(name);
          module.resolveAdditionalFile(name, makeDirectBuffer(content));
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
      return Vfs.read(file, progress);
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

