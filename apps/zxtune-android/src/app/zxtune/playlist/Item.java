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
import app.zxtune.Identifier;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;

public class Item {

  final private long id;
  final private Identifier location;
  final private String title;
  final private String author;
  final private TimeStamp duration;

  public Item(Cursor cursor) {
    this.id = cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal());
    this.location = Identifier.parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal()));
    this.title = cursor.getString(Database.Tables.Playlist.Fields.title.ordinal());
    this.author = cursor.getString(Database.Tables.Playlist.Fields.author.ordinal());
    this.duration = TimeStamp.createFrom(cursor.getInt(Database.Tables.Playlist.Fields.duration.ordinal()), TimeUnit.MILLISECONDS);
  }

  public Item(Identifier location, ZXTune.Module module) {
    this.id = -1;
    this.location = location;
    this.title = module.getProperty(ZXTune.Module.Attributes.TITLE, "");
    this.author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
    //TODO
    this.duration = TimeStamp.createFrom(module.getDuration() * 20, TimeUnit.MILLISECONDS);
  }
  
  final public Uri getUri() {
    return PlaylistQuery.uriFor(id);
  }
  
  final public Identifier getLocation() {
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
