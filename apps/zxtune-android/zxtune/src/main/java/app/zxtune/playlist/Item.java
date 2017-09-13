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

import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

import java.util.concurrent.TimeUnit;

import app.zxtune.Identifier;
import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;

public class Item {

  private final long id;
  private final Identifier location;
  private final String title;
  private final String author;
  private final TimeStamp duration;

  public Item(Cursor cursor) {
    this.id = cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal());
    this.location = Identifier.parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal()));
    this.title = cursor.getString(Database.Tables.Playlist.Fields.title.ordinal());
    this.author = cursor.getString(Database.Tables.Playlist.Fields.author.ordinal());
    this.duration = TimeStamp.createFrom(cursor.getInt(Database.Tables.Playlist.Fields.duration.ordinal()), TimeUnit.MILLISECONDS);
  }

  public Item(Identifier location, Module module) throws Exception {
    this.id = -1;
    this.location = location;
    this.title = module.getProperty(ModuleAttributes.TITLE, "");
    this.author = module.getProperty(ModuleAttributes.AUTHOR, "");
    //TODO
    this.duration = TimeStamp.createFrom(module.getDuration() * 20, TimeUnit.MILLISECONDS);
  }
  
  public final Uri getUri() {
    return PlaylistQuery.uriFor(id);
  }
  
  public final Identifier getLocation() {
    return location;
  }

  public final String getAuthor() {
    return author;
  }

  public final String getTitle() {
    return title;
  }

  public final TimeStamp getDuration() {
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
