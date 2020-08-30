package app.zxtune.playlist.xspf;

import android.util.Xml;

import androidx.annotation.Nullable;

import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.io.OutputStream;
import java.util.concurrent.TimeUnit;

import app.zxtune.playlist.Item;

class Builder {

  private final XmlSerializer xml;
  private boolean hasTracklist = false;

  Builder(OutputStream output) throws IOException {
    xml = Xml.newSerializer();
    xml.setOutput(output, Meta.ENCODING);
    xml.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
    xml.startDocument(Meta.ENCODING, true);
    xml.startTag(null, Tags.PLAYLIST)
        .attribute(null, Attributes.VERSION, Meta.XSPF_VERSION)
        .attribute(null, Attributes.XMLNS, Meta.XMLNS);
  }

  final void finish() throws IOException {
    if (hasTracklist) {
      xml.endTag(null, Tags.TRACKLIST);
    }
    xml.endTag(null, Tags.PLAYLIST);
    xml.endDocument();
    xml.flush();
  }

  final void writePlaylistProperties(String name, @Nullable Integer items) throws IOException {
    xml.startTag(null, Tags.EXTENSION)
        .attribute(null, Attributes.APPLICATION, Meta.APPLICATION);
    writeProperty(Properties.PLAYLIST_VERSION, Integer.toString(Meta.VERSION));
    writeProperty(Properties.PLAYLIST_NAME, name);
    if (items != null) {
      writeProperty(Properties.PLAYLIST_ITEMS, items.toString());
    }
    xml.endTag(null, Tags.EXTENSION);
  }

  final void writeTrack(Item item) throws IOException {
    if (!hasTracklist) {
      xml.startTag(null, Tags.TRACKLIST);
      hasTracklist = true;
    }
    xml.startTag(null, Tags.TRACK);
    writeTag(Tags.LOCATION, item.getLocation().toString());
    writeTag(Tags.CREATOR, item.getAuthor());
    writeTag(Tags.TITLE, item.getTitle());
    writeTag(Tags.DURATION, Long.toString(item.getDuration().convertTo(TimeUnit.MILLISECONDS)));
    //TODO: save extended properties
    xml.endTag(null, Tags.TRACK);
  }

  private void writeProperty(String name, String value) throws IOException {
    xml.startTag(null, Tags.PROPERTY)
        .attribute(null, Attributes.NAME, name)
        .text(value)
        .endTag(null, Tags.PROPERTY);
  }

  private void writeTag(String name, String value) throws IOException {
    if (0 != value.length()) {
      xml.startTag(null, name).text(value).endTag(null, name);
    }
  }
}
