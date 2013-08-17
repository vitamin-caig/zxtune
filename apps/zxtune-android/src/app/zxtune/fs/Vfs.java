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

import android.content.Context;

public final class Vfs {
  
  public static VfsRoot createRoot(Context context) {
    final VfsRootComposite composite = new VfsRootComposite();
    composite.addSubroot(new VfsRootLocal(context));
    return composite;
  }
}
