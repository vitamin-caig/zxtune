/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.archives;

import java.util.Arrays;

import android.text.TextUtils;
import app.zxtune.Identifier;

public class DirEntry {

  public final Identifier path;
  public final Identifier parent;
  public final String filename;
  
  DirEntry(Identifier path, Identifier parent, String filename) {
    this.path = path;
    this.parent = parent;
    this.filename = filename;
  }
  
  public final boolean isRoot() {
    return parent == null;
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
      return new DirEntry(id, null, null);
    } else {
      final String[] subpathElements = TextUtils.split(subpath, "/");
      final int subpathParts = subpathElements.length;
      final String[] parentSubpathElements = Arrays.copyOf(subpathElements, subpathParts - 1);
      final String parentSubpath = TextUtils.join("/", parentSubpathElements);
      final Identifier parent = new Identifier(id.getDataLocation(), parentSubpath);
      final String filename = subpathElements[subpathParts - 1];
      return new DirEntry(id, parent, filename);
    }
  }
}
