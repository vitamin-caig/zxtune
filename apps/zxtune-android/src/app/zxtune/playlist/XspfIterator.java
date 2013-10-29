/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playlist;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;

import org.xml.sax.SAXException;

import android.net.Uri;
import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.util.Xml;

public class XspfIterator {

  public static class Entry {
    public String location;
  }
  
  private final ArrayList<Entry> entries;
  private int current;
  
  public XspfIterator(ByteBuffer buf) throws IOException {
    this.entries = parse(buf);
    this.current = 0;
  }
  
  public final Entry getItem() {
    return entries.get(current);
  }
  
  public final boolean next() {
    if (current >= entries.size() - 1) {
      return false;
    } else {
      ++current;
      return true;
    }
  }
  
  public final boolean prev() {
    if (0 == current) {
      return false;
    } else {
      --current;
      return true;
    }
  }

  private ArrayList<Entry> parse(ByteBuffer buf) throws IOException {
    try {
      final ArrayList<Entry> result = new ArrayList<Entry>();
      final RootElement root = createPlaylistParseRoot(result);
      Xml.parse(newInputStream(buf), Xml.Encoding.UTF_8, root.getContentHandler());
      return result;
    } catch (SAXException e) {
      throw new IOException(e);
    }
  }
  
  private RootElement createPlaylistParseRoot(final ArrayList<Entry> entries) {
    final EntriesBuilder builder = new EntriesBuilder();
    final RootElement result = new RootElement(Xspf.XMLNS, Xspf.Tags.PLAYLIST);
    //TODO: check extension
    //final Element extension = result.getChild(Xspf.Tags.EXTENSION);
    final Element tracks = result.getChild(Xspf.XMLNS, Xspf.Tags.TRACKLIST);
    final Element track = tracks.getChild(Xspf.XMLNS, Xspf.Tags.TRACK);
    track.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Entry res = builder.captureResult();
        if (res != null) {
          entries.add(res);
        }
      }
    });
    track.getChild(Xspf.XMLNS, Xspf.Tags.LOCATION).setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setLocation(body);
      }
    });
    //TODO: parse rest properties
    return result;
  }

  private class EntriesBuilder {
    
    private Entry result;
    
    EntriesBuilder() {
      this.result = new Entry();
    }
    
    final void setLocation(String location) {
      result.location = Uri.decode(location);
    }
    
    final Entry captureResult() {
      final Entry res = result;
      result = new Entry();
      return res;
    }
  }
  
  private static InputStream newInputStream(final ByteBuffer buf) {
    return new InputStream() {
      
      @Override
      public int available() throws IOException {
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
      public int read() throws IOException {
        return buf.hasRemaining()
          ? buf.get()
          : -1;
      }
      
      @Override
      public int read(byte[] bytes, int off, int len) {
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
