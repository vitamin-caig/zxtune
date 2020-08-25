/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.util.Xml;

import androidx.annotation.Nullable;

import org.xmlpull.v1.XmlSerializer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.R;

public class XspfStorage {

  private static final String TAG = XspfStorage.class.getName();
  private static final String EXTENSION = ".xspf";
  private final File root;
  @Nullable
  private ArrayList<String> cache;

  public XspfStorage(Context context) {
    final String path = context.getString(R.string.playlists_storage_path);
    this.root = new File(Environment.getExternalStorageDirectory(), path);
    if (root.mkdirs()) {
      Log.d(TAG, "Created playlists storage dir");
    }
  }

  public final boolean isPlaylistExists(String name) {
    return -1 != getCachedPlaylists().indexOf(name);
  }

  public final ArrayList<String> enumeratePlaylists() {
    String[] files = root.list();
    if (files == null) {
      files = new String[0];
    }
    final ArrayList<String> result = new ArrayList<>(files.length);
    for (String filename : files) {
      final int extPos = filename.lastIndexOf(EXTENSION);
      if (-1 != extPos) {
        result.add(filename.substring(0, extPos));
      }
    }
    cache = result;
    return result;
  }

  private ArrayList<String> getCachedPlaylists() {
    return cache != null ? cache : enumeratePlaylists();
  }

  @Nullable
  public final File findPlaylistFile(String name) {
    final File res = getFileFor(name);
    return res.isFile() ? res : null;
  }

  private File getFileFor(String name) {
    return new File(root, name + EXTENSION);
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
    getCachedPlaylists().add(name);
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
