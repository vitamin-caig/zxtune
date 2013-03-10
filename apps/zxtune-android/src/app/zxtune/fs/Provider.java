/*
 * @file
 * 
 * @brief FS provider interface
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.fs;

import android.net.Uri;
import android.util.Log;
import java.util.ArrayList;

public class Provider {
  
  private static final String TAG = "app.zxtune.fs.Provider";

  private static final ArrayList<Vfs.Dir> roots = new ArrayList<Vfs.Dir>();
  
  static {
    LocalVfs.register();
  }
  
  static void registerVfs(Vfs.Dir root) {
    final Uri uri = root.uri();
    Log.d(TAG, String.format(" registered vfs '%s' name '%s'", uri.toString(), root.name())); 
    roots.add(root);
  }

  static Vfs.Dir enumerate() {
    return new CompositeDir(roots);
  }

  private static class CompositeDir implements Vfs.Dir {
    
    private final ArrayList<Vfs.Dir> roots;
    
    CompositeDir(ArrayList<Vfs.Dir> roots) {
      this.roots = roots;
    }

    public String name() {
      return null;
    }

    public Uri uri() {
      return null;
    }

    public Vfs.Entry[] list() {
      return roots.toArray(new Vfs.Entry[roots.size()]);
    }

    public Vfs.Entry resolve(Uri uri) {
      for (Vfs.Dir root : roots) {
        final Vfs.Entry entry = root.resolve(uri);
        if (entry != null) {
          return entry;
        }
      }
      return null;
    }
  }
}
