/**
 *
 * @file
 *
 * @brief Xspf file parser
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist.xspf;

import android.net.Uri;
import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.sax.TextElementListener;
import android.util.Xml;

import androidx.annotation.Nullable;

import org.xml.sax.SAXException;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.io.Io;
import app.zxtune.playlist.ReferencesArrayIterator;
import app.zxtune.playlist.ReferencesIterator;

public final class XspfIterator {
  
  public static ReferencesIterator create(ByteBuffer buf) throws IOException {
    return new ReferencesArrayIterator(parse(buf));
  }

  public static ArrayList<ReferencesIterator.Entry> parse(ByteBuffer buf) throws IOException {
    return parse(Io.createByteBufferInputStream(buf));
  }

  public static ArrayList<ReferencesIterator.Entry> parse(InputStream stream) throws IOException {
    try {
      final ArrayList<ReferencesIterator.Entry> result = new ArrayList<>();
      final RootElement root = createPlaylistParseRoot(result);
      Xml.parse(stream, Xml.Encoding.UTF_8, root.getContentHandler());
      return result;
    } catch (SAXException e) {
      throw new IOException(e);
    }
  }
  
  private static RootElement createPlaylistParseRoot(final ArrayList<ReferencesIterator.Entry> entries) {
    final EntriesBuilder builder = new EntriesBuilder();
    final RootElement result = new RootElement(Meta.XMLNS, Tags.PLAYLIST);
    //TODO: check extension
    final Element extension = result.getChild(Meta.XMLNS, Tags.EXTENSION);
    extension.getChild(Meta.XMLNS, Tags.PROPERTY).setTextElementListener(new TextElementListener() {
      
      private String propName;

      @Override
      public void start(org.xml.sax.Attributes attributes) {
        propName = attributes.getValue(Attributes.NAME);
      }

      @Override
      public void end(String body) {
        if (Properties.PLAYLIST_CREATOR.equals(propName)) {
          builder.desktopPaths = body.startsWith("zxtune-qt");
        } else if (Properties.PLAYLIST_VERSION.equals(propName)) {
          builder.escapedTexts = Integer.parseInt(body) < 2;
        }
      }
    });
    final Element tracks = result.getChild(Meta.XMLNS, Tags.TRACKLIST);
    final Element track = tracks.getChild(Meta.XMLNS, Tags.TRACK);
    track.setEndElementListener(() -> {
      final ReferencesIterator.Entry res = builder.captureResult();
      if (res != null) {
        entries.add(res);
      }
    });
    track.getChild(Meta.XMLNS, Tags.LOCATION).setEndTextElementListener(builder::setLocation);
    track.getChild(Meta.XMLNS, Tags.TITLE).setEndTextElementListener(builder::setTitle);
    track.getChild(Meta.XMLNS, Tags.CREATOR).setEndTextElementListener(builder::setCreator);
    track.getChild(Meta.XMLNS, Tags.DURATION).setEndTextElementListener(builder::setDuration);
    //TODO: parse rest properties
    return result;
  }

  private static class EntriesBuilder {
    
    private ReferencesIterator.Entry result;
    private boolean desktopPaths = false;
    private boolean escapedTexts = true;
    
    EntriesBuilder() {
      this.result = new ReferencesIterator.Entry();
    }
    
    final void setLocation(String location) {
      result.location = location;
      if (desktopPaths) {
        result.location = location
            //subpath for ay/sid containers
            .replace("?#", "#%23")
            //rest paths with subpath
            .replace('?', '#');
      } else {
        result.location = location;
      }
    }

    final void setTitle(String title) {
      result.title = decode(title);
    }

    final void setCreator(String author) {
      result.author = decode(author);
    }

    final void setDuration(String duration) {
      try {
        final long ms = Long.parseLong(duration);
        result.duration = TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS);
      } catch (NumberFormatException e) {
      }
    }

    private String decode(String str) {
      return escapedTexts ? Uri.decode(str) : str;
    }

    @Nullable
    final ReferencesIterator.Entry captureResult() {
      final ReferencesIterator.Entry res = result;
      result = new ReferencesIterator.Entry();
      return res.location != null ? res : null;
    }
  }

}
