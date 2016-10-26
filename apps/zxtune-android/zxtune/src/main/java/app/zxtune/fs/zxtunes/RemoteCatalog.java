/**
 *
 * @file
 *
 * @brief Remote implementation of catalog
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxtunes;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.nio.ByteBuffer;
import java.util.Locale;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.sax.StartElementListener;
import android.util.Xml;
import app.zxtune.Log;
import app.zxtune.fs.HttpProvider;

final class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static final String SITE = "http://zxtunes.com/";
  private static final String API = SITE + "xml.php?";
  private static final String ALL_AUTHORS_QUERY = API + "scope=authors&fields=nickname,name,tracks";
  //return nothing really, but required for more logical model
  private static final String ALL_TRACKS_QUERY = API
      + "scope=tracks&fields=filename,title,duration,date";
  private static final String AUTHOR_TRACKS_QUERY = ALL_TRACKS_QUERY + "&author_id=%d";
  private static final String DOWNLOAD_QUERY = SITE + "downloads.php?id=%d";

  private final HttpProvider http;

  public RemoteCatalog(HttpProvider http) {
    this.http = http;
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor) throws IOException {
    final HttpURLConnection connection = http.connect(ALL_AUTHORS_QUERY);
    final RootElement root = createAuthorsParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException {
    queryTracks(visitor, String.format(Locale.US, AUTHOR_TRACKS_QUERY, author.id));
  }

  private void queryTracks(TracksVisitor visitor, String query) throws IOException {
    final HttpURLConnection connection = http.connect(query);
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public boolean searchSupported() {
    return http.hasConnection();
  }
  
  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    throw new IOException("Search is not supported on remote side");
  }
  
  @Override
  public ByteBuffer getTrackContent(int id) throws IOException {
    try {
      final String query = String.format(Locale.US, DOWNLOAD_QUERY, id);
      return http.getContent(query);
    } catch (IOException e) {
      Log.w(TAG, e, "getModuleContent(%d)", id);
      throw e;
    }
  }

  private void performQuery(HttpURLConnection connection, RootElement root)
      throws IOException {
    try {
      final InputStream stream = new BufferedInputStream(connection.getInputStream());
      Xml.parse(stream, Xml.Encoding.UTF_8, root.getContentHandler());
    } catch (SAXException e) {
      throw new IOException(e);
    } catch (IOException e) {
      http.checkConnectionError();
      throw e;
    } finally {
      connection.disconnect();
    }
  }
  
  private static Integer asInt(String str) {
    if (str == null) {
      return null;
    } else try {
      return Integer.parseInt(str);
    } catch (NumberFormatException e) {
      return null;
    }
  }

  private static RootElement createAuthorsParserRoot(final AuthorsVisitor visitor) {
    final AuthorBuilder builder = new AuthorBuilder();
    final RootElement result = createRootElement();
    final Element list = result.getChild("authors");
    list.setStartElementListener(new StartElementListener() {
      @Override
      public void start(Attributes attributes) {
        final Integer count = asInt(attributes.getValue("count"));
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = list.getChild("author");
    item.setStartElementListener(new StartElementListener() {
      @Override
      public void start(Attributes attributes) {
        builder.setId(attributes.getValue("id"));
      }
    });
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Author res = builder.captureResult();
        if (res != null) {
          visitor.accept(res);
        }
      }
    });
    item.getChild("nickname").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setNickname(body);
      }
    });
    item.getChild("name").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setName(body);
      }
    });
    item.getChild("tracks").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setTracks(body);
      }
    });
    return result;
  }

  private static class AuthorBuilder {

    private Integer id;
    private String nickname;
    private String name;
    private Integer tracks;
    
    final void setId(String val) {
      id = Integer.valueOf(val);
    }

    final void setNickname(String val) {
      nickname = val;
    }

    final void setName(String val) {
      name = val;
    }

    final void setTracks(String val) {
      tracks = Integer.valueOf(val);
    }

    final Author captureResult() {
      final Author res = isValid() && hasTracks() ? new Author(id, nickname, name) : null;
      id = tracks = null;
      nickname = name = null;
      return res;
    }
    
    private boolean isValid() {
      return id != null && nickname != null;
    }
    
    private boolean hasTracks() {
      return tracks != null && tracks != 0;
    }
  }

  private static RootElement createModulesParserRoot(final TracksVisitor visitor) {
    final ModuleBuilder builder = new ModuleBuilder();
    final RootElement result = createRootElement();
    final Element list = result.getChild("tracks");
    list.setStartElementListener(new StartElementListener() {
      @Override
      public void start(Attributes attributes) {
        final Integer count = asInt(attributes.getValue("count"));
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = list.getChild("track");
    item.setStartElementListener(new StartElementListener() {
      @Override
      public void start(Attributes attributes) {
        builder.setId(attributes.getValue("id"));
      }
    });
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Track result = builder.captureResult();
        if (result != null) {
          visitor.accept(result);
        }
      }
    });
    item.getChild("filename").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setFilename(body);
      }
    });
    item.getChild("title").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setTitle(body);
      }
    });
    item.getChild("duration").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setDuration(body);
      }
    });
    item.getChild("date").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setDate(body);
      }
    });
    return result;
  }

  private static class ModuleBuilder {

    private Integer id;
    private String filename;
    private String title;
    private Integer duration;
    private Integer date;
    
    final void setId(String val) {
      id = Integer.valueOf(val);
    }

    final void setFilename(String val) {
      filename = val;
    }

    final void setTitle(String val) {
      title = val;
    }

    final void setDuration(String val) {
      duration = Integer.valueOf(val);
    }

    final void setDate(String val) {
      date = Integer.valueOf(val);
    }

    final Track captureResult() {
      final Track res = isValid() ? new Track(id, filename, title, duration, date) : null;
      id = duration = date = null;
      filename = title = null;
      return res;
    }
    
    private boolean isValid() {
      return id != null && filename != null;
    }
  }

  private static RootElement createRootElement() {
    //TODO: check root tag version
    return new RootElement("zxtunes");
  }
}
