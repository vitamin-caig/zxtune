/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import android.net.Uri;
import android.sax.Element;
import android.sax.RootElement;
import android.util.Xml;

import androidx.annotation.Nullable;

import org.xml.sax.SAXException;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.http.MultisourceHttpProvider;

public final class RemoteCatalog implements Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static final String SITE = "http://zxtunes.com/";
  private static final String API = SITE + "xml.php?";
  private static final String ALL_AUTHORS_QUERY = API + "scope=authors&fields=nickname,name,tracks";
  //return nothing really, but required for more logical model
  private static final String ALL_TRACKS_QUERY = API
          + "scope=tracks&fields=filename,title,duration,date";
  private static final String AUTHOR_TRACKS_QUERY = ALL_TRACKS_QUERY + "&author_id=%d";
  private static final String DOWNLOAD_QUERY = SITE + "downloads.php?id=%d";

  private final MultisourceHttpProvider http;

  RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors()");
    final InputStream stream = http.getInputStream(Uri.parse(ALL_AUTHORS_QUERY));
    final RootElement root = createAuthorsParserRoot(visitor);
    performQuery(stream, root);
  }

  @Override
  public void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthorTracks(author=%d)", author.getId());
    queryTracks(visitor, String.format(Locale.US, AUTHOR_TRACKS_QUERY, author.getId()));
  }

  private void queryTracks(TracksVisitor visitor, String query) throws IOException {
    final InputStream stream = http.getInputStream(Uri.parse(query));
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(stream, root);
  }

  @Override
  public boolean searchSupported() {
    return http.hasConnection();
  }

  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    throw new IOException("Search is not supported on remote side");
  }

  public static Uri[] getTrackUris(int id) {
    final String query = String.format(Locale.US, DOWNLOAD_QUERY, id);
    return new Uri[]{Uri.parse(query)};
  }

  private void performQuery(InputStream httpStream, RootElement root)
          throws IOException {
    try {
      final InputStream stream = new BufferedInputStream(httpStream);
      Xml.parse(stream, Xml.Encoding.UTF_8, root.getContentHandler());
    } catch (SAXException e) {
      throw new IOException(e);
    } finally {
      httpStream.close();
    }
  }

  private static RootElement createAuthorsParserRoot(final AuthorsVisitor visitor) {
    final AuthorBuilder builder = new AuthorBuilder();
    final RootElement result = createRootElement();
    final Element list = result.getChild("authors");
    list.setStartElementListener(attributes -> {
      final Integer count = HtmlUtils.tryGetInteger(attributes.getValue("count"));
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = list.getChild("author");
    item.setStartElementListener(attributes -> builder.setId(attributes.getValue("id")));
    item.setEndElementListener(() -> {
      final Author res = builder.captureResult();
      if (res != null) {
        visitor.accept(res);
      }
    });
    item.getChild("nickname").setEndTextElementListener(builder::setNickname);
    item.getChild("name").setEndTextElementListener(builder::setName);
    item.getChild("tracks").setEndTextElementListener(builder::setTracks);
    return result;
  }

  private static class AuthorBuilder {

    @Nullable
    private Integer id;
    @Nullable
    private String nickname;
    private String name = "";
    @Nullable
    private Integer tracks;

    AuthorBuilder() {
      reset();
    }

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

    @Nullable
    final Author captureResult() {
      final Author res = isValid() && hasTracks() ? new Author(id, nickname, name) : null;
      reset();
      return res;
    }

    private void reset() {
      id = null;
      tracks = null;
      nickname = null;
      name = "";
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
    list.setStartElementListener(attributes -> {
      final Integer count = HtmlUtils.tryGetInteger(attributes.getValue("count"));
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = list.getChild("track");
    item.setStartElementListener(attributes -> builder.setId(attributes.getValue("id")));
    item.setEndElementListener(() -> {
      final Track result1 = builder.captureResult();
      if (result1 != null) {
        visitor.accept(result1);
      }
    });
    item.getChild("filename").setEndTextElementListener(builder::setFilename);
    item.getChild("title").setEndTextElementListener(builder::setTitle);
    item.getChild("duration").setEndTextElementListener(builder::setDuration);
    item.getChild("date").setEndTextElementListener(builder::setDate);
    return result;
  }

  private static class ModuleBuilder {

    @Nullable
    private Integer id;
    @Nullable
    private String filename;
    @Nullable
    private String title;
    @Nullable
    private Integer duration;
    @Nullable
    private Integer date;

    ModuleBuilder() {
      reset();
    }

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

    @Nullable
    final Track captureResult() {
      final Track res = isValid() ? new Track(id, filename, title, duration, date) : null;
      reset();
      return res;
    }

    private void reset() {
      id = null;
      duration = null;
      date = null;
      filename = null;
      title = "";
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
