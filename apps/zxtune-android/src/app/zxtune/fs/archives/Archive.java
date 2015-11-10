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

import android.database.Cursor;
import android.net.Uri;

public class Archive {
  
  public final Uri path;
  public final int modules;

  Archive(Uri path, int modules) {
    this.path = path;
    this.modules = modules;
  }
  
  public static Archive fromCursor(Cursor cursor) {
    final Uri path = Uri.parse(cursor.getString(Database.Tables.Archives.Fields.path.ordinal()));
    final int modules = cursor.getInt(Database.Tables.Archives.Fields.modules.ordinal());
    return new Archive(path, modules);
  }
}
