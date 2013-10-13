/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.StringTokenizer;

import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.text.TextUtils;
import android.text.format.Formatter;
import app.zxtune.R;

@VfsDir.Icon(R.drawable.ic_browser_vfs_local)
final class VfsRootLocal implements VfsRoot {
  
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

  @Override
  public void enumerate(Visitor visitor) {
    for (File root : File.listRoots()) {
      visitor.onDir(new LocalDir(root, root.getAbsolutePath()));
    }
    for (String storage : getExternalStorageDirectories()) {
      visitor.onDir(buildDir(new File(storage)));
    }
  }

  @Override
  public void find(String mask, Visitor visitor) {
    //TODO
  }
  
  //Based on code from http://renzhi.ca/2012/02/03/how-to-list-all-sd-cards-on-android/
  private static List<String> getExternalStorageDirectories() {
    try {
      return readMountPoints("/proc/mounts");
    }
    catch (IOException e) {
    }
    return Collections.emptyList();
  }
  
  private static List<String> readMountPoints(String filename) throws IOException {
    final BufferedReader bufReader = new BufferedReader(new FileReader(filename));
    try {
      return readMountPoints(bufReader);
    } finally {
      bufReader.close();
    }
  }
  
  private static List<String> readMountPoints(BufferedReader reader) throws IOException {
    final ArrayList<String> list = new ArrayList<String>();
    for (;;) {
      final String line = reader.readLine();
      if (line == null) {
        break;
      }
      if (line.contains("vfat") || line.contains("/mnt")) {
        StringTokenizer tokens = new StringTokenizer(line, " ");
        String s = tokens.nextToken();
        s = tokens.nextToken(); // Take the second token, i.e. mount point

        if (s.equals(Environment.getExternalStorageDirectory().getPath())) {
          list.add(s);
        }
        else if (line.contains("/dev/block/vold")
          && !(line.contains("/mnt/secure") || line.contains("/mnt/asec") || 
               line.contains("/mnt/obb") || line.contains("/dev/mapper") || 
               line.contains("tmpfs"))) {
          list.add(s);
        }
      }
    }
    return list;
  }

  private class LocalDir implements VfsDir {

    private final File dir;
    private final String name;

    public LocalDir(File dir) {
      this(dir, dir.getName());
    }

    public LocalDir(File dir, String name) {
      assert dir.isDirectory();
      this.dir = dir;
      this.name = name;
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
    public String getDescription() {
      return "".intern();
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      final File[] files = dir.listFiles();
      if (files == null) {
        throw new IOException("Failed to enumerate files in directory");
      }
      for (File file : files) {
        if (file.isDirectory()) {
          visitor.onDir(buildDir(file));
        } else if (file.isFile()) {
          visitor.onFile(buildFile(file));
        }
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      //TODO
    }
  }

  private class LocalFile implements VfsFile {

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
    public String getDescription() {
      return "".intern();
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, file.length());
    }

    @Override
    public byte[] getContent() throws IOException {
      final FileInputStream stream = new FileInputStream(file);
      try {
        final int size = (int) file.length();
        byte[] result = new byte[size];
        stream.read(result, 0, size);
        return result;
      } finally {
        stream.close();
      }
    }
  }
}
