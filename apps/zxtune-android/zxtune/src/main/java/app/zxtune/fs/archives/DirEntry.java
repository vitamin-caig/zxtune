/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.archives;

import android.text.TextUtils;

import app.zxtune.core.Identifier;

public class DirEntry {

  public final Identifier path;
  public final Identifier parent;
  public final String filename;

  private DirEntry(Identifier path, Identifier parent, String filename) {
    this.path = path;
    this.parent = parent;
    this.filename = filename;
  }

  public final boolean isRoot() {
    return filename.isEmpty();
  }

  public static DirEntry create(Identifier id) {
    final String subpath = id.getSubpath();
    if (TextUtils.isEmpty(subpath)) {
      /*
       * TODO: fix model to avoid conflicts in the next case:
       * 
       * file.ext - module at the beginning
       * file.exe?+100 - module at the offset
       */
      return new DirEntry(id, id, "");
    } else {
      final int lastSeparatorPos = subpath.lastIndexOf('/');
      if (lastSeparatorPos != -1) {
        final String parentSubpath = subpath.substring(0, lastSeparatorPos);
        final Identifier parent = new Identifier(id.getDataLocation(), parentSubpath);
        final String filename = subpath.substring(lastSeparatorPos + 1);
        return new DirEntry(id, parent, filename);
      } else {
        final Identifier parent = new Identifier(id.getDataLocation(), "");
        return new DirEntry(id, parent, subpath);
      }
    }
  }
}
