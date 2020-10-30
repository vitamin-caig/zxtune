/**
 * @file
 * @brief Database (metadata) item
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playlist;

import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

import app.zxtune.core.Identifier;
import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;

import java.util.concurrent.TimeUnit;

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

  public Item(Identifier location, Module module) {
    this.id = -1;
    this.location = location;
    this.title = module.getProperty(ModuleAttributes.TITLE, "");
    this.author = module.getProperty(ModuleAttributes.AUTHOR, "");
    this.duration = module.getDuration();
  }

  public Item(Identifier location, String title, String author, TimeStamp duration) {
    this.id = -1;
    this.location = location;
    this.title = title;
    this.author = author;
    this.duration = duration;
  }

  public Item(app.zxtune.playback.Item item) {
    this.id = -1;
    this.location = item.getDataId();
    this.title = item.getTitle();
    this.author = item.getAuthor();
    this.duration = item.getDuration();
  }

  public Item() {
    this.id = -1;
    this.location = Identifier.EMPTY;
    this.title = "";
    this.author = "";
    this.duration = TimeStamp.EMPTY;
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
