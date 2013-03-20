/*
 * @file
 * @brief Playlist item
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.playlist;

import android.database.Cursor;

public class Item {

  final private String type;
  final private String title;
  final private String author;
  final private String duration;

  public Item(Cursor cursor) {
    type = cursor.getString(Database.Tables.Playlist.Fields.type.ordinal());
    final String title = cursor.getString(Database.Tables.Playlist.Fields.title.ordinal());
    author = cursor.getString(Database.Tables.Playlist.Fields.author.ordinal());
    final boolean hasTitleOrAuthor = 0 != title.length() + author.length();
    this.title =
        hasTitleOrAuthor ? title : cursor.getString(Database.Tables.Playlist.Fields.uri.ordinal());
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
