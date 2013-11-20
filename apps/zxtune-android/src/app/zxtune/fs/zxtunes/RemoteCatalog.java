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
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Locale;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.sax.StartElementListener;
import android.util.Log;
import android.util.Xml;
import app.zxtune.R;

final class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static final String SITE = "http://www.zxtunes.com/";
  private static final String API = SITE + "xml.php?";
  private static final String ALL_AUTHORS_QUERY = API + "scope=authors&fields=nickname,name,tracks";
  private static final String AUTHOR_QUERY = ALL_AUTHORS_QUERY + "&id=%d";
  //return nothing really, but required for more logical model
  private static final String ALL_TRACKS_QUERY = API
      + "scope=tracks&fields=filename,title,duration,date";
  private static final String AUTHOR_TRACKS_QUERY = ALL_TRACKS_QUERY + "&author_id=%d";
  private static final String TRACK_QUERY = ALL_TRACKS_QUERY + "&id=%d";
  private static final String DOWNLOAD_QUERY = SITE + "downloads.php?id=%d";

  private final Context context;
  private String userAgent;

  public RemoteCatalog(Context context) {
    this.context = context;
    this.userAgent =
        String.format("%s/%s", context.getString(R.string.app_name),
            context.getString(R.string.versionName));
    Log.d(TAG, "Set useragent to " + userAgent);
    // HTTP connection reuse which was buggy pre-froyo
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.FROYO) {
      System.setProperty("http.keepAlive", "false");
    }
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor, Integer id) throws IOException {
    final String query =
        id == null ? ALL_AUTHORS_QUERY : String.format(Locale.US, AUTHOR_QUERY, id);
    final HttpURLConnection connection = connect(query);
    final RootElement root = createAuthorsParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public void queryTracks(TracksVisitor visitor, Integer id, Integer author) throws IOException {
    if (id != null) {
      queryTracks(visitor, String.format(Locale.US, TRACK_QUERY, id));
    } else {
      queryTracks(visitor, String.format(Locale.US, AUTHOR_TRACKS_QUERY, author));
    }
  }

  private void queryTracks(TracksVisitor visitor, String query) throws IOException {
    final HttpURLConnection connection = connect(query);
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public ByteBuffer getTrackContent(int id) throws IOException {
    try {
      final String query = String.format(Locale.US, DOWNLOAD_QUERY, id);
      final HttpURLConnection connection = connect(query);
      return getContent(connection);
    } catch (IOException e) {
      Log.d(TAG, "getModuleContent(" + id + ")", e);
      throw hasConnection()
        ? e
        : new IOException(context.getString(R.string.network_inaccessible));
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
      throw hasConnection()
        ? e
        : new IOException(context.getString(R.string.network_inaccessible));
      } finally {
      connection.disconnect();
    }
  }

  private static RootElement createAuthorsParserRoot(final AuthorsVisitor visitor) {
    final AuthorBuilder builder = new AuthorBuilder();
    final RootElement result = createRootElement();
    final Element list = result.getChild("authors");
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
      final Author res = tracks != null && tracks != 0 ? new Author(id, nickname, name) : null;
      id = tracks = null;
      nickname = null;
      name = "".intern();//ok for null
      return res;
    }
  }

  private static RootElement createModulesParserRoot(final TracksVisitor visitor) {
    final ModuleBuilder builder = new ModuleBuilder();
    final RootElement result = createRootElement();
    final Element list = result.getChild("tracks");
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
        visitor.accept(builder.captureResult());
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
      final Track res = new Track(id, filename, title, duration, date);
      id = date = null;
      duration = Integer.valueOf(0);//stub for null
      filename = null;
      title = "".intern();//stub for null
      return res;
    }
  }

  private static RootElement createRootElement() {
    //TODO: check root tag version
    return new RootElement("zxtunes");
  }

  private static ByteBuffer getContent(HttpURLConnection connection) throws IOException {
    try {
      final int len = connection.getContentLength();
      final InputStream stream = connection.getInputStream();
      final ByteBuffer result = ByteBuffer.allocateDirect(len);
      if (result.hasArray()) {
        readContent(stream, result.array());
      } else {
        final byte[] buffer = new byte[len];
        readContent(stream, buffer);
        result.put(buffer);
        result.rewind();
      }
      return result;
    } finally {
      connection.disconnect();
    }
  }

  private static void readContent(InputStream stream, byte[] buffer) throws IOException {
    final int len = buffer.length;
    int received = 0;
    for (;;) {
      final int chunk = stream.read(buffer, received, len - received);
      if (chunk <= 0) {
        break;
      }
      received += chunk;
    }
    if (len != received) {
      throw new IOException(String.format(
          "Read content size mismatch (%d received, %d expected)", received, len));
    }
  }

  private HttpURLConnection connect(String uri) throws IOException {
    try {
      final URL url = new URL(uri);
      final HttpURLConnection result = (HttpURLConnection) url.openConnection();
      result.setRequestProperty("User-Agent", userAgent);
      Log.d(TAG, String.format("Fetch %d bytes via %s", result.getContentLength(), uri));
      return result;
    } catch (IOException e) {
      Log.d(TAG, "Fetch " + uri, e);
      throw e;
    }
  }
  
  private boolean hasConnection() {
    final ConnectivityManager mgr = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    final NetworkInfo info = mgr.getActiveNetworkInfo();
    return info != null && info.isConnected();
  }
}
