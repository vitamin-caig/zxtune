/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.net.Uri;
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
import java.net.HttpURLConnection;
import java.nio.ByteBuffer;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.Util;
import app.zxtune.fs.HttpProvider;

final class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  //no www. prefix!!!
  private static final String SITE = "http://zxart.ee";
  private static final String API = SITE + "/zxtune/language:eng";
  private static final String ACTION_AUTHORS = "/action:authors";
  private static final String ACTION_PARTIES = "/action:parties";
  private static final String ACTION_TRACKS = "/action:tunes";
  private static final String ACTION_TOP = "/action:topTunes";
  private static final String ACTION_SEARCH = "/action:search";
  private static final String LIMIT = "/limit:%d";
  private static final String AUTHOR_ID = "/authorId:%d";
  private static final String PARTY_ID = "/partyId:%d";
  //private static final String TRACK_ID = "/tuneId:%d";
  private static final String ALL_TRACKS_QUERY = API + ACTION_TRACKS;
  private static final String DOWNLOAD_QUERY = SITE + "/file/id:%d";
  private static final String ALL_AUTHORS_QUERY = API + ACTION_AUTHORS;
  private static final String AUTHOR_TRACKS_QUERY = ALL_TRACKS_QUERY + AUTHOR_ID;
  private static final String ALL_PARTIES_QUERY = API + ACTION_PARTIES;
  private static final String PARTY_TRACKS_QUERY = ALL_TRACKS_QUERY + PARTY_ID;
  private static final String TOP_TRACKS_QUERY = API + ACTION_TOP + LIMIT;
  private static final String FIND_TRACKS_QUERY = API + ACTION_SEARCH + "/query:%s";

  private final HttpProvider http;

  public RemoteCatalog(HttpProvider http) {
    this.http = http;
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors()");
    final HttpURLConnection connection = http.connect(ALL_AUTHORS_QUERY);
    final RootElement root = createAuthorsParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public void queryAuthorTracks(Author author, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthorTracks(author=%d)", author.id);
    queryTracks(visitor, String.format(Locale.US, AUTHOR_TRACKS_QUERY, author.id));
  }

  @Override
  public void queryParties(PartiesVisitor visitor) throws IOException {
    Log.d(TAG, "queryParties()");
    final HttpURLConnection connection = http.connect(ALL_PARTIES_QUERY);
    final RootElement root = createPartiesParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  public void queryPartyTracks(Party party, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryPartyTracks(party=%d)", party.id);
    queryTracks(visitor, String.format(Locale.US, PARTY_TRACKS_QUERY, party.id));
  }

  @Override
  public void queryTopTracks(int limit, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryTopTracks()");
    queryTracks(visitor, String.format(Locale.US, TOP_TRACKS_QUERY, limit));
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

  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    Log.d(TAG, "findTracks(query=%s)", query);
    final String url = String.format(Locale.US, FIND_TRACKS_QUERY, Uri.encode(query));
    final HttpURLConnection connection = http.connect(url);
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(connection, root);
  }

  @Override
  @NonNull
  public ByteBuffer getTrackContent(int id) throws IOException {
    Log.d(TAG, "getTrackContent(id=%d)", id);
    final String query = String.format(Locale.US, DOWNLOAD_QUERY, id);
    return http.getContent(query);
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

  private static RootElement createAuthorsParserRoot(final AuthorsVisitor visitor) {
    final AuthorBuilder builder = new AuthorBuilder();
    final RootElement result = createRootElement();
    final Element data = result.getChild("responseData");
    data.getChild("totalAmount").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer count = asInt(body);
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = data.getChild("authors").getChild("author");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Author res = builder.captureResult();
        if (res != null) {
          visitor.accept(res);
        }
      }
    });
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    item.getChild("title").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setNickname(body);
      }
    });
    item.getChild("realName").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setName(body);
      }
    });
    return result;
  }

  private static class AuthorBuilder {

    private Integer id;
    private String nickname;
    private String name;

    final void setId(String val) {
      id = Integer.valueOf(val);
    }

    final void setNickname(String val) {
      nickname = val;
    }

    final void setName(String val) {
      name = val;
    }

    @Nullable
    final Author captureResult() {
      final Author res = isValid() ? new Author(id, nickname, name) : null;
      id = null;
      nickname = name = null;
      return res;
    }

    private boolean isValid() {
      return id != null && nickname != null;
    }
  }

  private static RootElement createPartiesParserRoot(final PartiesVisitor visitor) {
    final PartiesBuilder builder = new PartiesBuilder();
    final RootElement result = createRootElement();
    final Element data = result.getChild("responseData");
    data.getChild("totalAmount").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer count = asInt(body);
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = data.getChild("parties").getChild("party");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Party res = builder.captureResult();
        if (res != null) {
          visitor.accept(res);
        }
      }
    });
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    item.getChild("title").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setName(body);
      }
    });
    item.getChild("year").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setYear(body);
      }
    });
    return result;
  }

  private static class PartiesBuilder {

    private Integer id;
    private String name;
    private int year;

    final void setId(String val) {
      id = Integer.valueOf(val);
    }

    final void setName(String val) {
      name = val;
    }

    final void setYear(String val) {
      try {
        year = Integer.valueOf(val);
      } catch (NumberFormatException e) {
        year = 0;
      }
    }

    @Nullable
    final Party captureResult() {
      final Party res = isValid() ? new Party(id, name, year) : null;
      id = null;
      name = null;
      year = 0;
      return res;
    }

    private boolean isValid() {
      return id != null && name != null;
    }
  }

  private static RootElement createModulesParserRoot(final TracksVisitor visitor) {
    final ModuleBuilder builder = new ModuleBuilder();
    final RootElement result = createRootElement();
    final Element data = result.getChild("responseData");
    data.getChild("totalAmount").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer count = asInt(body);
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = data.getChild("tunes").getChild("tune");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Track result = builder.captureResult();
        if (result != null) {
          visitor.accept(result);
        }
      }
    });
    bindXmlActions(item, builder);
    return result;
  }

  //TODO: remove C&P
  private static RootElement createModulesParserRoot(final FoundTracksVisitor visitor) {
    final ModuleBuilder builder = new ModuleBuilder();
    final RootElement result = createRootElement();
    final Element data = result.getChild("responseData");
    data.getChild("totalAmount").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        final Integer count = asInt(body);
        if (count != null) {
          visitor.setCountHint(count);
        }
      }
    });
    final Element item = data.getChild("tunes").getChild("tune");
    item.setEndElementListener(new EndElementListener() {
      @Override
      public void end() {
        final Author author = builder.captureResultAuthor();
        final Track result = builder.captureResult();
        if (result != null && author != null) {
          visitor.accept(author, result);
        }
      }
    });
    bindXmlActions(item, builder);
    return result;
  }

  private static void bindXmlActions(Element item, final ModuleBuilder builder) {
    item.getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setId(body);
      }
    });
    //CDATA
    item.getChild("originalFileName").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setFilename(Html.fromHtml(body).toString());
      }
    });
    item.getChild("title").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setTitle(body);
      }
    });
    item.getChild("internalAuthor").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setInternalAuthor(body);
      }
    });
    item.getChild("internalTitle").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setInternalTitle(body);
      }
    });
    item.getChild("votes").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setVotes(body);
      }
    });
    item.getChild("time").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setDuration(body);
      }
    });
    item.getChild("year").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setYear(body);
      }
    });
    item.getChild("compo").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setCompo(body);
      }
    });
    item.getChild("partyplace").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setPartyplace(body);
      }
    });
    item.getChild("authors").getChild("id").setEndTextElementListener(new EndTextElementListener() {
      @Override
      public void end(String body) {
        builder.setAuthorId(body);
      }
    });
  }

  private static class ModuleBuilder {

    private Integer id;
    private String filename;
    private String title;
    private String internalAuthor;
    private String internalTitle;
    private String votes;
    private String duration;
    private int year;
    private String compo;
    private int partyplace;
    private Integer authorId;

    ModuleBuilder() {
      reset();
    }

    final void setId(String val) {
      id = Integer.valueOf(val);
    }

    final void setFilename(String val) {
      filename = val.trim();
      if (filename.length() == 0) {
        filename = "unknown";
      }
    }

    final void setTitle(String val) {
      title = val;
    }

    final void setInternalAuthor(String val) {
      internalAuthor = val;
    }

    final void setInternalTitle(String val) {
      internalTitle = val;
    }

    final void setVotes(String val) {
      votes = val;
    }

    final void setDuration(String val) {
      duration = val;
    }

    final void setYear(String val) {
      try {
        year = Integer.valueOf(val);
      } catch (NumberFormatException e) {
        year = 0;
      }
    }

    final void setCompo(String val) {
      compo = val;
    }

    final void setPartyplace(String val) {
      try {
        partyplace = Integer.valueOf(val);
      } catch (NumberFormatException e) {
        partyplace = 0;
      }
    }

    final void setAuthorId(String val) {
      try {
        authorId = Integer.valueOf(val);
      } catch (NumberFormatException e) {
        authorId = null;
      }
    }

    @Nullable
    final Author captureResultAuthor() {
      if (authorId != null) {
        final String fakeName = "Author" + authorId;
        final Author res = new Author(authorId, fakeName, "");
        authorId = null;
        return res;
      } else {
        return null;
      }
    }

    @Nullable
    final Track captureResult() {
      title = Util.formatTrackTitle(internalAuthor, internalTitle, title);
      final Track res = isValid()
              ? new Track(id, filename, title, votes, duration, year, compo, partyplace)
              : null;
      reset();
      return res;
    }

    private void reset() {
      id = null;
      filename = null;
      year = partyplace = 0;
      votes = duration = title = internalAuthor = internalTitle = compo = "";
      authorId = null;
    }

    private boolean isValid() {
      return id != null && filename != null;
    }
  }

  private static RootElement createRootElement() {
    return new RootElement("response");
  }
}
