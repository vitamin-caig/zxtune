/*
 * @file
 * 
 * @brief Vfs interfaces
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.fs;

import android.net.Uri;

public final class Vfs {

  public interface Entry {

    public String name();

    public Uri uri();
  }

  public interface File extends Entry {

    public long size();
  }

  public interface Dir extends Entry {

    public Entry[] list();
    
    public Entry resolve(Uri uri);
  }
  
  public static Dir getRoot() {
    return Provider.enumerate();
  }
}
