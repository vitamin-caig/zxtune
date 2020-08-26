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
    final boolean[] pathCompatMode = {false};
    final EntriesBuilder builder = new EntriesBuilder();
    final RootElement result = new RootElement(Meta.XMLNS, Tags.PLAYLIST);
    //TODO: check extension
    final Element extension = result.getChild(Meta.XMLNS, Tags.EXTENSION);
    extension.getChild(Meta.XMLNS, Tags.PROPERTY).setTextElementListener(new TextElementListener() {
      
      private boolean isCreatorProperty;

      @Override
      public void start(org.xml.sax.Attributes attributes) {
        final String propName = attributes.getValue(Attributes.NAME);
        isCreatorProperty = Properties.PLAYLIST_CREATOR.equals(propName);
      }

      @Override
      public void end(String body) {
        if (isCreatorProperty) {
          pathCompatMode[0] = body.startsWith("zxtune-qt");
        }
      }
    });
    final Element tracks = result.getChild(Meta.XMLNS, Tags.TRACKLIST);
    final Element track = tracks.getChild(Meta.XMLNS, Tags.TRACK);
    track.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final ReferencesIterator.Entry res = builder.captureResult();
        if (res != null) {
          entries.add(res);
        }
      }
    });
    track.getChild(Meta.XMLNS, Tags.LOCATION).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        if (pathCompatMode[0]) {
          body = body
              //subpath for ay/sid containers
              .replace("?#", "#%23")
              //rest paths with subpath
              .replace('?', '#');
        }
        builder.setLocation(body);
      }
    });
    track.getChild(Meta.XMLNS, Tags.TITLE).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setTitle(body);
      }
    });
    track.getChild(Meta.XMLNS, Tags.CREATOR).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setCreator(body);
      }
    });
    track.getChild(Meta.XMLNS, Tags.DURATION).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setDuration(body);
      }
    });
    //TODO: parse rest properties
    return result;
  }

  private static class EntriesBuilder {
    
    private ReferencesIterator.Entry result;
    
    EntriesBuilder() {
      this.result = new ReferencesIterator.Entry();
    }
    
    final void setLocation(String location) {
      result.location = location;
    }

    final void setTitle(String title) {
      result.title = Uri.decode(title);
    }

    final void setCreator(String author) {
      result.author = Uri.decode(author);
    }

    final void setDuration(String duration) {
      try {
        final long ms = Long.parseLong(duration);
        result.duration = TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS);
      } catch (NumberFormatException e) {
      }
    }

    @Nullable
    final ReferencesIterator.Entry captureResult() {
      final ReferencesIterator.Entry res = result;
      result = new ReferencesIterator.Entry();
      return res.location != null ? res : null;
    }
  }

}
