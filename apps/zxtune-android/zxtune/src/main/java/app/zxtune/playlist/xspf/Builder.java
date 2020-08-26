package app.zxtune.playlist.xspf;

import android.database.Cursor;
import android.net.Uri;

import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import app.zxtune.playlist.Item;

class Builder {

  private final XmlSerializer xml;

  Builder(XmlSerializer xml) throws IOException {
    this.xml = xml;
    xml.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
    xml.startDocument(Meta.ENCODING, true);
    xml.startTag(null, Tags.PLAYLIST)
        .attribute(null, Attributes.VERSION, Meta.VERSION)
        .attribute(null, Attributes.XMLNS, Meta.XMLNS);
  }

  final void flush() throws IOException {
    xml.endTag(null, Tags.PLAYLIST);
    xml.endDocument();
    xml.flush();
  }

  final void writePlaylistProperties(String name, int items) throws IOException {
    xml.startTag(null, Tags.EXTENSION)
        .attribute(null, Attributes.APPLICATION, Meta.APPLICATION);
    writeProperty(Properties.PLAYLIST_VERSION, Meta.VERSION);
    writeProperty(Properties.PLAYLIST_NAME, name);
    writeProperty(Properties.PLAYLIST_ITEMS, Integer.toString(items));
    xml.endTag(null, Tags.EXTENSION);
  }

  final void writeTracks(Cursor cursor) throws IOException {
    xml.startTag(null, Tags.TRACKLIST);
    while (cursor.moveToNext()) {
      writeTrack(new Item(cursor));
    }
    xml.endTag(null, Tags.TRACKLIST);
  }

  private void writeTrack(Item item) throws IOException {
    xml.startTag(null, Tags.TRACK);
    writeTag(Tags.LOCATION, item.getLocation().toString());
    writeTextTag(Tags.CREATOR, item.getAuthor());
    writeTextTag(Tags.TITLE, item.getTitle());
    writeTextTag(Tags.DURATION, Long.toString(item.getDuration().convertTo(TimeUnit.MILLISECONDS)));
    //TODO: save extended properties
    xml.endTag(null, Tags.TRACK);
  }

  private void writeProperty(String name, String value) throws IOException {
    xml.startTag(null, Tags.PROPERTY)
        .attribute(null, Attributes.NAME, name)
        .text(Uri.encode(value))
        .endTag(null, Tags.PROPERTY);
  }

  private void writeTextTag(String name, String value) throws IOException {
    writeTag(name, Uri.encode(value));
  }

  private void writeTag(String name, String value) throws IOException {
    if (0 != value.length()) {
      xml.startTag(null, name).text(value).endTag(null, name);
    }
  }
}
