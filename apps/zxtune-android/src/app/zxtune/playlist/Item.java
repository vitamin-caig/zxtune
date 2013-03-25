/*
 * @file
 * @brief Playlist item
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.playlist;

import android.database.Cursor;
import android.net.Uri;

public class Item {

  final private long id;
  final private String dataUri;
  final private String type;
  final private String title;
  final private String author;
  final private String duration;

  public Item(Cursor cursor) {
    id = cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal());
    dataUri = cursor.getString(Database.Tables.Playlist.Fields.uri.ordinal());
    type = cursor.getString(Database.Tables.Playlist.Fields.type.ordinal());
    title = cursor.getString(Database.Tables.Playlist.Fields.title.ordinal());
    author = cursor.getString(Database.Tables.Playlist.Fields.author.ordinal());
    duration = formatDuration(cursor.getInt(Database.Tables.Playlist.Fields.duration.ordinal()));
  }
  
  private static String formatDuration(int totalMs) {
    final int totalSec = totalMs / 1000;
    final int totalMin = totalSec / 60;
    final int totalHour = totalMin / 60;
    final int min = totalMin % 60;
    final int sec = totalSec % 60;
    return totalHour != 0 ? String.format("%d:%d:%d", totalHour, min, sec) : String.format("%d:%02d", min, sec);
  }
  
  public Uri getUri() {
    return Query.unparse(id);
  }
  
  public String getDataUri() {
    return dataUri;
  }

  public String getType() {
    return type;
  }

  public String getAuthor() {
    return author;
  }

  public String getTitle() {
    return title;
  }

  public String getDuration() {
    return duration;
  }
}
