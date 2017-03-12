/**
 *
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 *
 */
package app.zxtune.playlist;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.util.Xml;

import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InvalidObjectException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.R;

public class XspfStorage {
  
  private final static String EXTENSION = ".xspf"; 
  private final File root;
  private final ArrayList<String> names;

  public XspfStorage(Context context) {
    final String path = context.getString(R.string.playlists_storage_path);
    this.root = new File(Environment.getExternalStorageDirectory() + File.separator + path);
    root.mkdirs();
    this.names = new ArrayList<String>();

    fillNames();
  }
  
  private void fillNames() {
    String[] files = root.list();
    if (files == null) {
      files = new String[0];
    }
    for (int i = 0; i != files.length; ++i) {
      final String filename = files[i];
      final int extPos = filename.lastIndexOf(EXTENSION);
      if (-1 != extPos) {
        names.add(filename.substring(0, extPos));
      }
    }
  }

  public final boolean isPlaylistExists(String name) {
    return -1 != names.indexOf(name);
  }
  
  public final ArrayList<String> enumeratePlaylists() {
    return names;
  }
  
  public final Uri getPlaylistUri(String name) throws InvalidObjectException {
    if (!isPlaylistExists(name)) {
      throw new InvalidObjectException(name);
    }
    return Uri.fromFile(getFileFor(name));
  }
  
  private File getFileFor(String name) {
    return new File(root + File.separator + name + EXTENSION);
  }
  
  public final void createPlaylist(String name, Cursor cursor) throws IOException {
    final File file = getFileFor(name);
    final FileOutputStream stream = new FileOutputStream(file);
    final XmlSerializer xml = Xml.newSerializer();
    xml.setOutput(stream, Xspf.ENCODING);
    final Builder builder = new Builder(xml);
    builder.writePlaylistProperties(cursor.getCount());
    builder.writeTracks(cursor);
    builder.flush();
    names.add(name);
  }

  private static class Builder {

    private final XmlSerializer xml;

    Builder(XmlSerializer xml) throws IOException {
      this.xml = xml;
      xml.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
      xml.startDocument(Xspf.ENCODING, true);
      xml.startTag(null, Xspf.Tags.PLAYLIST)
       .attribute(null, Xspf.Attributes.VERSION, Xspf.VERSION)
       .attribute(null, Xspf.Attributes.XMLNS, Xspf.XMLNS);
    }

    final void flush() throws IOException {
      xml.endTag(null, Xspf.Tags.PLAYLIST);
      xml.endDocument();
      xml.flush();
    }

    final void writePlaylistProperties(int items) throws IOException {
      xml.startTag(null, Xspf.Tags.EXTENSION)
        .attribute(null, Xspf.Attributes.APPLICATION, Xspf.APPLICATION);
      writeProperty(Xspf.Properties.PLAYLIST_VERSION, Xspf.VERSION);
      writeProperty(Xspf.Properties.PLAYLIST_ITEMS, Integer.toString(items));
      xml.endTag(null, Xspf.Tags.EXTENSION);
    }

    final void writeTracks(Cursor cursor) throws IOException {
      xml.startTag(null, Xspf.Tags.TRACKLIST);
      while (cursor.moveToNext()) {
        writeTrack(new Item(cursor));
      }
      xml.endTag(null, Xspf.Tags.TRACKLIST);
    }

    private void writeTrack(Item item) throws IOException {
      xml.startTag(null, Xspf.Tags.TRACK);
      writeTag(Xspf.Tags.LOCATION, item.getLocation().toString());
      writeTextTag(Xspf.Tags.CREATOR, item.getAuthor());
      writeTextTag(Xspf.Tags.TITLE, item.getTitle());
      writeTextTag(Xspf.Tags.DURATION, Long.toString(item.getDuration().convertTo(TimeUnit.MILLISECONDS)));
      //TODO: save extended properties
      xml.endTag(null, Xspf.Tags.TRACK);
    }

    private void writeProperty(String name, String value) throws IOException {
      xml.startTag(null, Xspf.Tags.PROPERTY)
        .attribute(null, Xspf.Attributes.NAME, name)
        .text(Uri.encode(value))
        .endTag(null, Xspf.Tags.PROPERTY);
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
}
