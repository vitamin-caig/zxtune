/**
 *
 * @file
 *
 * @brief VFS entry point
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import android.content.Context;

public final class Vfs {

  public static VfsRoot createRoot(Context context) {
    final VfsRootComposite composite = new VfsRootComposite();
    composite.addSubroot(new VfsRootLocal(context));
    composite.addSubroot(new VfsRootZxtunes(context));
    composite.addSubroot(new VfsRootPlaylists(context));
    return composite;
  }
}
