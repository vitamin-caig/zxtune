/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.sax.Element;
import android.sax.EndElementListener;
import android.sax.EndTextElementListener;
import android.sax.RootElement;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.Html;
import android.util.Xml;

import org.xml.sax.SAXException;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Log;
import app.zxtune.fs.HttpProvider;

/**
 * API entry point:
 *   http://api.modarchive.org
 *
 * Authors:
 *   ${API}/xml-tools.php?key=${key}&request=search_artist&page=${page}
 *
 * references author's tracks at
 *   ${API}/xml-tools.php?key=${key}&request=view_modules_by_artistid&query=${id}&page=${page}
 *
 * references tracks at
 *   ${API}/downloads.php?moduleid=${moduleid}
 *
 * Genres:
 *   ${API}/xml-tools.php?key=${key}&request=view_genres (no paging)
 *
 * references genre's tracks at
 *   ${API}/xml-tools.php?key=${key}&request=search&type=genre&query=${genreid}&page=${page}
 *
 * Search:
 *   ${API}/xml-tools.php?key=${key}&request=search&type=filename_or_songtitle&query=*${query}*&page=${page}
 */

class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static class ApiUriBuilder {
    private final Uri.Builder delegate;

    private ApiUriBuilder() {
      delegate = new Uri.Builder();
      delegate.scheme("http");
      delegate.authority("api.modarchive.org");
    }

    final ApiUriBuilder setRequest(String request) {
      delegate.appendQueryParameter("request", request);
      return this;
    }

    final ApiUriBuilder setQuery(String query) {
      delegate.appendQueryParameter("query", query);
      return this;
    }

    final ApiUriBuilder setQuery(int query) {
      return setQuery(Integer.toString(query));
    }

    final ApiUriBuilder setType(String type) {
      delegate.appendQueryParameter("type", type);
      return this;
    }

    final String build() {
      return delegate.build().toString();
    }

    static ApiUriBuilder forQuery(String key) {
      final ApiUriBuilder result = new ApiUriBuilder();
      result.delegate.path("xml-tools.php");
      result.delegate.appendQueryParameter("key", key);
      return result;
    }

    static ApiUriBuilder forDownload(int trackId) {
      final ApiUriBuilder result = new ApiUriBuilder();
      result.delegate.path("downloads.php");
      result.delegate.appendQueryParameter("moduleid", Integer.toString(trackId));
      return result;
    }
  }

  private final HttpProvider http;
  private final String key;

  RemoteCatalog(Context context, HttpProvider http) {
    this.http = http;
    final Bundle metaData = getAppMetadata(context);
    this.key = metaData.getString("key.modarchive");
  }

  private static Bundle getAppMetadata(Context context) {
    try {
      final ApplicationInfo info = context.getPackageManager().getApplicationInfo(context.getPackageName(), PackageManager.GET_META_DATA);
      return info.metaData;
    } catch (NameNotFoundException e) {
      return new Bundle();
    }
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors()");
    final String uri = ApiUriBuilder.forQuery(key).setRequest("search_artist").build();
    final RootElement root = createAuthorsParserRoot(visitor);
    loadPages(uri, root);
    visitor.accept(Author.UNKNOWN);
  }

  @Override
  public void queryGenres(GenresVisitor visitor) throws IOException {
    Log.d(TAG, "queryGenres()");
    final String uri = ApiUriBuilder.forQuery(key).setRequest("view_genres").build();
    final RootElement root = createGenresParserRoot(visitor);
    loadSinglePage(uri, root);
  }

  @Override
  public void queryTracks(Author author, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryTracks(author=%d)", author.id);
    final String uri = ApiUriBuilder.forQuery(key).setRequest("view_modules_by_artistid").setQuery(author.id).build();
    final RootElement root = createTracksParserRoot(visitor);
    loadPages(uri, root);
  }

  @Override
  public void queryTracks(Genre genre, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryTracks(genre=%d)", genre.id);
    final String uri = ApiUriBuilder.forQuery(key).setRequest("search").setType("genre").setQuery(genre.id).build();
    final RootElement root = createTracksParserRoot(visitor);
    loadPages(uri, root);
  }

  @Override
  public boolean searchSupported() {
    return http.hasConnection();
  }

  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    Log.d(TAG, "findTracks(query=%s)", query);
    final String uri = ApiUriBuilder.forQuery(key).setRequest("search").setType("filename_or_songtitle").setQuery("*" + query + "*").build();
    final RootElement root = createFoundTracksParserRoot(visitor);
    loadPages(uri, root);
  }

  @Override
  @NonNull
  public ByteBuffer getTrackContent(int id) throws IOException {
    Log.d(TAG, "getTrackContent(id=%d)", id);
    final String query = ApiUriBuilder.forDownload(id).build();
    return http.getContent(query);
  }

  final void getTrackContent(int id, OutputStream stream) throws IOException {
    Log.d(TAG, "getTrackContent(id=%d)", id);
    final String query = ApiUriBuilder.forDownload(id).build();
    http.getContent(query, stream);
  }

  private static RootElement createAuthorsParserRoot(final AuthorsVisitor visitor) {
    final AuthorBuilder builder = new AuthorBuilder();
    final RootElement root = createRootElement();
    root.getChild("total_results").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer result = asInt(body);
        if (result != null) {
          visitor.setCountHint(result);
        }
      }
    });
    final Element item = root.getChild("items").getChild("item");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Author res = builder.captureResult();
        if (res != null) {
          visitor.accept(res);
        }
      }
    });
    bindAuthorFields(item, builder);
    return root;
  }

  private static class AuthorBuilder {
    private Integer id;
    private String alias;

    final void setId(String val) {
      id = asInt(val);
    }

    final void setAlias(String val) {
      alias = val;
    }

    @Nullable
    final Author captureResult() {
      final Author res = isValid() ? new Author(id, alias) : null;
      id = null;
      alias = null;
      return res;
    }

    private boolean isValid() {
      return id != null && alias != null;
    }
  }

  private static RootElement createGenresParserRoot(final GenresVisitor visitor) {
    final GenreBuilder builder = new GenreBuilder();
    final RootElement root = createRootElement();
    root.getChild("results").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer result = asInt(body);
        if (result != null) {
          visitor.setCountHint(result);
        }
      }
    });
    final Element item = root.getChild("parent").getChild("children").getChild("child");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Genre result = builder.captureResult();
        if (result != null) {
          visitor.accept(result);
        }
      }
    });
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    item.getChild("text").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setText(body);
      }
    });
    item.getChild("files").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setFiles(body);
      }
    });
    return root;
  }

  private static class GenreBuilder {
    private Integer id;
    private String text;
    private Integer files;

    final void setId(String val) {
      id = asInt(val);
    }

    final void setText(String val) {
      text = val;
    }

    final void setFiles(String val) {
      files = asInt(val);
    }

    @Nullable
    final Genre captureResult() {
      final Genre res = isValid() ? new Genre(id, text, files) : null;
      id = files = null;
      text = null;
      return res;
    }

    private boolean isValid() {
      return id != null && text != null && files != null;
    }
  }

  private static RootElement createTracksParserRoot(final TracksVisitor visitor) {
    final TrackBuilder builder = new TrackBuilder();
    final RootElement root = createRootElement();
    root.getChild("total_results").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer result = asInt(body);
        if (result != null) {
          visitor.setCountHint(result);
        }
      }
    });
    final Element item = root.getChild("module");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Track result = builder.captureResult();
        if (result != null) {
          visitor.accept(result);
        }
      }
    });
    bindTrackFields(item, builder);
    return root;
  }

  private static class TrackBuilder {
    private Integer id;
    private String filename;
    private String title;
    private Integer size;

    final void setId(String val) {
      id = asInt(val);
    }

    final void setFilename(String val) {
      filename = val;
    }

    final void setTitle(String val) {
      title = val;
    }

    final void setSize(String val) {
      size = asInt(val);
    }

    @Nullable
    final Track captureResult() {
      final Track res = isValid() ? new Track(id, filename, title, size) : null;
      id = size = null;
      filename = title = null;
      return res;
    }

    private boolean isValid() {
      return id != null && filename != null && title != null && size != null;
    }
  }

  private static RootElement createFoundTracksParserRoot(final FoundTracksVisitor visitor) {
    final TrackBuilder trackBuilder = new TrackBuilder();
    final AuthorBuilder authorBuilder = new AuthorBuilder();
    final RootElement root = createRootElement();
    root.getChild("results").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer result = asInt(body);
        if (result != null) {
          visitor.setCountHint(result);
        }
      }
    });
    final Element item = root.getChild("module");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Track track = trackBuilder.captureResult();
        final Author author = authorBuilder.captureResult();
        if (track != null) {
          visitor.accept(author != null ? author : Author.UNKNOWN, track);
        }
      }
    });
    bindTrackFields(item, trackBuilder);
    bindAuthorFields(item.getChild("artist_info").getChild("artist"), authorBuilder);
    return root;
  }

  private static void bindTrackFields(Element item, final TrackBuilder builder) {
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    item.getChild("filename").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setFilename(body);
      }
    });
    item.getChild("bytes").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setSize(body);
      }
    });
    item.getChild("songtitle").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        //CDATA
        builder.setTitle(Html.fromHtml(body).toString());
      }
    });
  }

  private static void bindAuthorFields(Element item, final AuthorBuilder builder) {
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    item.getChild("alias").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setAlias(body);
      }
    });
  }

  private void loadPages(String baseUri, RootElement root) throws IOException {
    final AtomicInteger totalPages = new AtomicInteger(1);
    root.getChild("totalpages").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer result = asInt(body);
        if (totalPages.get() == 1 && result != null) {
          Log.d(TAG, "Loading %d pages", result);
          totalPages.set(result);
        }
      }
    });
    for (int page = 1; page <= totalPages.get(); ++page) {
      final String pageUri = baseUri + "&page=" + Integer.toString(page);
      loadSinglePage(pageUri, root);
    }
  }

  private void loadSinglePage(String uri, RootElement root) throws IOException {
    final HttpURLConnection connection = http.connect(uri);
    performXmlQuery(connection, root);
  }

  private void performXmlQuery(HttpURLConnection connection, RootElement root) throws IOException {
    try {
      final InputStream stream = new BufferedInputStream(connection.getInputStream());
      try {
        Xml.parse(stream, Xml.Encoding.UTF_8, root.getContentHandler());
      } finally {
        stream.close();
      }
    } catch (SAXException e) {
      throw new IOException(e);
    } catch (IOException e) {
      http.checkConnectionError();
      throw e;
    } finally {
      connection.disconnect();
    }
  }

  @Nullable
  private static Integer asInt(String str) {
    if (str == null) {
      return null;
    } else try {
      return Integer.parseInt(str);
    } catch (NumberFormatException e) {
      return null;
    }
  }

  private static RootElement createRootElement() {
    return new RootElement("modarchive");
  }
}
