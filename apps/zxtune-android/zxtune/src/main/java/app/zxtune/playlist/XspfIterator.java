/**
 *
 * @file
 *
 * @brief Xspf file parser
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.net.Uri;
import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.sax.TextElementListener;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.util.Xml;

import app.zxtune.TimeStamp;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

public final class XspfIterator {
  
  public static ReferencesIterator create(ByteBuffer buf) throws IOException {
    return new ReferencesArrayIterator(parse(buf));
  }

  public static ArrayList<ReferencesIterator.Entry> parse(ByteBuffer buf) throws IOException {
    try {
      final ArrayList<ReferencesIterator.Entry> result = new ArrayList<>();
      final RootElement root = createPlaylistParseRoot(result);
      Xml.parse(newInputStream(buf), Xml.Encoding.UTF_8, root.getContentHandler());
      return result;
    } catch (SAXException e) {
      throw new IOException(e);
    }
  }
  
  private static RootElement createPlaylistParseRoot(final ArrayList<ReferencesIterator.Entry> entries) {
    final boolean[] pathCompatMode = {false};
    final EntriesBuilder builder = new EntriesBuilder();
    final RootElement result = new RootElement(Xspf.XMLNS, Xspf.Tags.PLAYLIST);
    //TODO: check extension
    final Element extension = result.getChild(Xspf.XMLNS, Xspf.Tags.EXTENSION);
    extension.getChild(Xspf.XMLNS, Xspf.Tags.PROPERTY).setTextElementListener(new TextElementListener() {
      
      private boolean isCreatorProperty;

      @Override
      public void start(Attributes attributes) {
        final String propName = attributes.getValue(Xspf.Attributes.NAME);
        isCreatorProperty = Xspf.Properties.PLAYLIST_CREATOR.equals(propName);
      }

      @Override
      public void end(String body) {
        if (isCreatorProperty) {
          pathCompatMode[0] = body.startsWith("zxtune-qt");
        }
      }
    });
    final Element tracks = result.getChild(Xspf.XMLNS, Xspf.Tags.TRACKLIST);
    final Element track = tracks.getChild(Xspf.XMLNS, Xspf.Tags.TRACK);
    track.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final ReferencesIterator.Entry res = builder.captureResult();
        if (res != null) {
          entries.add(res);
        }
      }
    });
    track.getChild(Xspf.XMLNS, Xspf.Tags.LOCATION).setEndTextElementListener(new EndTextElementListener() {
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
    track.getChild(Xspf.XMLNS, Xspf.Tags.TITLE).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setTitle(body);
      }
    });
    track.getChild(Xspf.XMLNS, Xspf.Tags.CREATOR).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setCreator(body);
      }
    });
    track.getChild(Xspf.XMLNS, Xspf.Tags.DURATION).setEndTextElementListener(new EndTextElementListener() {
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
        final Long ms = Long.valueOf(duration);
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
  
  static InputStream newInputStream(final ByteBuffer buf) {
    return new InputStream() {
      
      @Override
      public int available()  {
        return buf.remaining();
      }
      
      @Override
      public void mark(int readLimit) {
        buf.mark();
      }
      
      @Override
      public void reset() {
        buf.reset();
      }
      
      @Override
      public boolean markSupported() {
        return true;
      }
      
      @Override
      public int read() {
        return buf.hasRemaining()
          ? buf.get()
          : -1;
      }
      
      @Override
      public int read(@NonNull byte[] bytes, int off, int len) {
        if (buf.hasRemaining()) {
          len = Math.min(len, buf.remaining());
          buf.get(bytes, off, len);
          return len;
        } else {
          return -1;
        }
      }
    };
  }
}
