/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.archives;

import android.database.Cursor;
import android.net.Uri;
import androidx.annotation.Nullable;

import java.util.concurrent.TimeUnit;

import app.zxtune.core.Identifier;
import app.zxtune.TimeStamp;

//POJO presentation of entries table
public class Entry {

  public final DirEntry dirEntry;
  @Nullable
  public final Track track;

  private Entry(Identifier path, String description, @Nullable TimeStamp duration) {
    this.dirEntry = DirEntry.create(path);
    this.track = dirEntry.filename != null && duration != null
            ? new Track(dirEntry.path.getFullLocation(), dirEntry.filename, description, duration)
            : null;
  }

  public static Entry fromCursor(Cursor cursor) {
    final Identifier path = new Identifier(Uri.parse(cursor.getString(Database.Tables.Entries.Fields.path.ordinal())));
    final String description = cursor.getString(Database.Tables.Entries.Fields.description.ordinal());
    final long durationValue = cursor.getInt(Database.Tables.Entries.Fields.duration.ordinal());
    final TimeStamp duration = durationValue != 0
            ? TimeStamp.createFrom(durationValue, TimeUnit.MILLISECONDS)
            : null;
    return new Entry(path, description, duration);
  }
}
