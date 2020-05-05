/**
 * @file
 * @brief Implementation of VfsRoot over local files
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.annotation.TargetApi;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.text.TextUtils;
import android.text.format.Formatter;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Scanner;

import app.zxtune.Log;
import app.zxtune.R;

@Icon(R.drawable.ic_browser_vfs_local)
final class VfsRootLocal extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootLocal.class.getName();
  private static final String SCHEME = "file";

  private final Context context;

  VfsRootLocal(Context context) {
    this.context = context;
  }

  @Override
  public Uri getUri() {
    return buildUri("");
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_local_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_local_root_description);
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return null;
  }

  @Override
  public void enumerate(Visitor visitor) {
    if (Build.VERSION.SDK_INT < 24 || !listStorageVolumes(visitor)) {
      listMountpoints(visitor);
    }
    listPlaces(visitor);
  }

  @TargetApi(24)
  private boolean listStorageVolumes(Visitor visitor) {
    final StorageManager mgr = (StorageManager)context.getSystemService(Context.STORAGE_SERVICE);
    if (mgr == null) {
      return false;
    }
    try {
      final Method method = StorageVolume.class.getMethod("getPathFile");
      for (StorageVolume vol : mgr.getStorageVolumes()) {
        if (isGoodState(vol.getState())) {
          feedGood((File) method.invoke(vol), vol.getDescription(context), visitor);
        }
      }
      return true;
    } catch (IllegalAccessException e) {
      Log.w(TAG, e, "Failed to get path");
    } catch (InvocationTargetException e) {
      Log.w(TAG, e, "Failed to get path");
    } catch (NoSuchMethodException e) {
      Log.w(TAG, e, getStorageVolumeMethods());
    }
    return false;
  }

  private boolean isGoodState(String state) {
    return Environment.MEDIA_MOUNTED.equals(state) || Environment.MEDIA_MOUNTED_READ_ONLY.equals(state);
  }

  @TargetApi(24)
  private static String getStorageVolumeMethods() {
    final StringBuilder builder = new StringBuilder();
    builder.append("Available get methods:");
    for (Method m : StorageVolume.class.getMethods()) {
      final String name = m.getName();
      if (name.startsWith("get")) {
        builder.append(' ');
        builder.append(name);
      }
    }
    return builder.toString();
  }

  private void listMountpoints(Visitor visitor) {
    final File mounts = new File("/proc/mounts");
    ArrayList<File> result = new ArrayList<>(1);
    try {
      final Scanner scan = new Scanner(mounts);
      while (scan.hasNext()) {
        //assume no comments in file
        final String line = scan.nextLine();
        final String[] fields = line.split(" ");
        final String device = fields[0];
        if (device.startsWith("/dev/block/vold")
                || device.equals("/dev/fuse")) {
          final String mountpoint = fields[1];
          final File point = new File(mountpoint);
          if (!result.contains(point)) {
            result.add(point);
          }
        }
      }
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to get external storages");
    }
    if (result.isEmpty()) {
      if (isGoodState(Environment.getExternalStorageState())) {
        feedGood(Environment.getExternalStorageDirectory(), "External storage", visitor);
      } else {
        for (File root : File.listRoots()) {
          feedGood(root, "Root filesystem", visitor);
        }
      }
    } else {
      for (File dir : result) {
        feedGood(dir, visitor);
      }
    }
  }

  private void listPlaces(Visitor visitor) {
    final String[] types = {Environment.DIRECTORY_MUSIC, Environment.DIRECTORY_PODCASTS};
    for (String type : types) {
      feedGood(Environment.getExternalStoragePublicDirectory(type), visitor);
    }
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      final String path = uri.getPath();
      if (TextUtils.isEmpty(path)) {
        return this;
      } else {
        return resolvePath(path);
      }
    } else {
      return null;
    }
  }

  private static Uri buildUri(String path) {
    return new Uri.Builder().scheme(SCHEME).path(path).build();
  }

  @Nullable
  private VfsObject buildObject(File obj) {
    if (obj.isDirectory()) {
      return buildDir(obj);
    } else if (obj.isFile()) {
      return buildFile(obj);
    } else {
      return null;
    }
  }

  private VfsDir buildDir(File obj) {
    return new LocalDir(obj);
  }

  private VfsDir buildDir(File obj, String description) {
    return new LocalDir(obj, description);
  }

  private VfsFile buildFile(File obj) {
    return new LocalFile(obj);
  }

  private void feedGood(File dir, Visitor visitor) {
    if (dir != null) {
      visitor.onDir(buildDir(dir));
    }
  }

  private void feedGood(File dir, String description, Visitor visitor) {
    //dir.canRead sometimes returns false while dir is really readable
    if (dir != null) {
      visitor.onDir(buildDir(dir, description));
    }
  }

  @Nullable
  private VfsObject resolvePath(String path) {
    final File obj = new File(path);
    return buildObject(obj);
  }

  private class LocalObject extends StubObject implements VfsObject {

    final File object;
    final String name;

    LocalObject(File obj, String name) {
      this.object = obj;
      this.name = name;
    }

    @Override
    public Uri getUri() {
      return buildUri(object.getPath());
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public VfsObject getParent() {
      final File parent = object.getParentFile();
      return parent != null
              ? new LocalDir(parent)
              : VfsRootLocal.this;
    }
  }

  private static String getDirName(File dir) {
    final String name = dir.getName();
    return 0 != name.length() ? name : dir.getAbsolutePath();
  }

  private class LocalDir extends LocalObject implements VfsDir {

    private final String description;

    LocalDir(File dir) {
      super(dir, getDirName(dir));
      this.description = "";
    }

    LocalDir(File dir, String description) {
      super(dir, getDirName(dir));
      this.description = description;
    }

    @Override
    public String getDescription() {
      return description;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      final File[] files = object.listFiles();
      if (files == null) {
        throw new IOException("Failed to enumerate files at " + object.getAbsolutePath());
      }
      visitor.onItemsCount(files.length);
      for (File file : files) {
        if (file.isDirectory()) {
          visitor.onDir(buildDir(file));
        } else if (file.isFile()) {
          visitor.onFile(buildFile(file));
        }
      }
    }
  }

  private class LocalFile extends LocalObject implements VfsFile {

    LocalFile(File file) {
      super(file, file.getName());
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, object.length());
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.FILE.equals(id)) {
        return object;
      } else {
        return super.getExtension(id);
      }
    }
  }
}
