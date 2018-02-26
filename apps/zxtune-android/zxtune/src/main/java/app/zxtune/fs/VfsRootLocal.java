/**
 * @file
 * @brief Implementation of VfsRoot over local files
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.text.format.Formatter;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import app.zxtune.Log;
import app.zxtune.R;

final class VfsRootLocal extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootLocal.class.getName();
  private static final String SCHEME = "file";

  private final Context context;

  public VfsRootLocal(Context context) {
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
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_local;
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (File root : File.listRoots()) {
      visitor.onDir(buildDir(root));
    }
    for (File storage : getExternalStorages()) {
      visitor.onDir(buildDir(storage));
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

  private VfsFile buildFile(File obj) {
    return new LocalFile(obj);
  }

  @Nullable
  private VfsObject resolvePath(String path) {
    final File obj = new File(path);
    return buildObject(obj);
  }

  private static List<File> getExternalStorages() {
    final List<File> result = new ArrayList<>(5);
    final File mounts = new File("/proc/mounts");
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
          if (point.exists() && point.canRead() && !result.contains(point)) {
            result.add(point);
          }
        }
      }
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to get external storages");
    }
    if (result.isEmpty()) {
      result.add(Environment.getExternalStorageDirectory());
    }
    return result;
  }

  private class LocalObject extends StubObject implements VfsObject {

    final File object;
    final String name;

    public LocalObject(File obj, String name) {
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

    public LocalDir(File dir) {
      super(dir, getDirName(dir));
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

    public LocalFile(File file) {
      super(file, file.getName());
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, object.length());
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return Io.readFrom(object);
    }
  }
}
