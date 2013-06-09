/*
 * @file
 * 
 * @brief LocalVfs implementation
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.fs;

import java.io.File;

import android.net.Uri;
import android.os.Environment;
import android.util.Log;

public final class LocalVfs {

  private final static String TAG = "app.zxtune.fs.LocalVfs";
  private final static String SCHEME = "file";
  
  static void register() {
    Log.d(TAG, "Registering LocalVfs entries:");
    for (File root : File.listRoots()) {
      Provider.registerVfs(new LocalDir(root, root.getAbsolutePath()));
    }
    {
      final String state = Environment.getExternalStorageState();
      if (state.equals(Environment.MEDIA_MOUNTED) || state.equals(Environment.MEDIA_MOUNTED_READ_ONLY)) {
        final File dir = Environment.getExternalStorageDirectory();
        Provider.registerVfs(new LocalDir(dir)); 
      } else {
        Log.d(TAG, " external storage in unsupported state " + state);
      }
    }
  }
  
  static Vfs.Entry[] buildList(File[] entries) {

    final Vfs.Entry[] res = new Vfs.Entry[entries.length];

    for (int idx = 0; idx != entries.length; ++idx) {
      res[idx] = buildEntry(entries[idx]);
    }
    return res;
  }

  static Vfs.Entry buildEntry(File entry) {
    if (entry.isDirectory()) {
      return new LocalDir(entry);
    } else if (entry.isFile()) {
      return new LocalFile(entry);
    } else {
      return null;
    }
  }

  private static Uri buildUri(String path) {
    return new Uri.Builder().scheme(SCHEME).path(path).build();
  }

  private static class LocalDir implements Vfs.Dir {

    private final File dir;
    private final String name;

    public LocalDir(File dir) {
      this(dir, dir.getName());
    }
    
    public LocalDir(File dir, String name) {
      this.dir = dir;
      this.name = name;
    }

    public String name() {
      return name;
    }

    public Uri uri() {
      return buildUri(dir.getPath());
    }

    public Vfs.Entry[] list() {
      final File[] files = dir.listFiles();
      return files != null ? buildList(files) : null;
    }

    public Vfs.Entry resolve(Uri uri) {
      if (!uri.getScheme().equals(SCHEME)) {
        return null;
      }
      final File obj = new File(uri.getPath());
      return buildEntry(obj);
    }
  }

  private static class LocalFile implements Vfs.File {

    private final File file;

    public LocalFile(File file) {
      this.file = file;
    }

    public String name() {
      return file.getName();
    }

    public Uri uri() {
      return buildUri(file.getPath());
    }

    public long size() {
      return file.length();
    }
  }
}
