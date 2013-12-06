/**
 *
 * @file
 *
 * @brief Database (metadata) item
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import java.util.concurrent.TimeUnit;

import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;

public class Item {

  final private long id;
  final private Uri location;
  final private String title;
  final private String author;
  final private TimeStamp duration;

  public Item(Cursor cursor) {
    id = cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal());
    location = Uri.parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal()));
    title = cursor.getString(Database.Tables.Playlist.Fields.title.ordinal());
    author = cursor.getString(Database.Tables.Playlist.Fields.author.ordinal());
    duration = TimeStamp.createFrom(cursor.getInt(Database.Tables.Playlist.Fields.duration.ordinal()), TimeUnit.MILLISECONDS);
  }

  public Item(Uri uri, ZXTune.Module module) {
    id = -1;
    location = uri;
    title = module.getProperty(ZXTune.Module.Attributes.TITLE, "");
    author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
    //TODO
    duration = TimeStamp.createFrom(module.getDuration() * 20, TimeUnit.MILLISECONDS);
  }
  
  final public Uri getUri() {
    return PlaylistQuery.uriFor(id);
  }
  
  final public Uri getLocation() {
    return location;
  }

  final public String getAuthor() {
    return author;
  }

  final public String getTitle() {
    return title;
  }

  final public TimeStamp getDuration() {
    return duration;
  }

  public ContentValues toContentValues() {
    final ContentValues values = new ContentValues();
    values.put(Database.Tables.Playlist.Fields.location.name(), location.toString());
    values.put(Database.Tables.Playlist.Fields.author.name(), author);
    values.put(Database.Tables.Playlist.Fields.title.name(), title);
    values.put(Database.Tables.Playlist.Fields.duration.name(), duration.convertTo(TimeUnit.MILLISECONDS));
    return values;
  }
}
