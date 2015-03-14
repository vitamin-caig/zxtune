/**
 *
 * @file
 *
 * @brief Implementation of VfsRoot over local files
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.text.TextUtils;
import android.text.format.Formatter;
import app.zxtune.R;
import app.zxtune.ui.IconSource;

final class VfsRootLocal implements VfsRoot, IconSource {
  
  private final static String SCHEME = "file";
  
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
  public VfsDir getParent() {
    return null;
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
  
  @Override
  public int getResourceId() {
    return R.drawable.ic_browser_vfs_local;
  }

  private static Uri buildUri(String path) {
    return new Uri.Builder().scheme(SCHEME).path(path).build(); 
  }
  
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
  
  private VfsObject resolvePath(String path) {
    final File obj = new File(path);
    return buildObject(obj);
  }

  
  private List<File> getExternalStorages() {
    final List<File> result = new ArrayList<File>(5);
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
    }
    if (result.isEmpty()) {
      result.add(Environment.getExternalStorageDirectory());
    }
    return result;
  }

  private class LocalDir extends StubObject implements VfsDir {

    private final File dir;
    private final String name;

    public LocalDir(File dir) {
      assert dir.isDirectory();
      this.dir = dir;
      final String name = dir.getName();
      this.name = 0 != name.length() ? name : dir.getAbsolutePath(); 
    }

    @Override
    public Uri getUri() {
      return buildUri(dir.getPath());
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public VfsDir getParent() {
      final File parent = dir.getParentFile();
      return parent != null
        ? new LocalDir(parent)
        : VfsRootLocal.this;
    }
    
    @Override
    public void enumerate(Visitor visitor) throws IOException {
      final File[] files = dir.listFiles();
      if (files == null) {
        throw new IOException("Failed to enumerate files in directory");
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

  private class LocalFile extends StubObject implements VfsFile {

    private final File file;

    public LocalFile(File file) {
      assert file.isFile();
      this.file = file;
    }

    @Override
    public Uri getUri() {
      return buildUri(file.getPath());
    }

    @Override
    public String getName() {
      return file.getName();
    }
    
    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, file.length());
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      final FileInputStream stream = new FileInputStream(file);
      try {
        final FileChannel channel = stream.getChannel();
        try {
          return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
        } finally {
          channel.close();
        }
      } finally {
        stream.close();
      }
    }
  }
}
