/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.net.Uri;
import android.sax.Element;
import android.sax.RootElement;
import android.text.Html;
import android.util.Xml;

import androidx.annotation.Nullable;

import org.xml.sax.SAXException;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.Util;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.http.MultisourceHttpProvider;

public final class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  //no www. prefix!!!
  private static final String SITE = "https://zxart.ee";
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

  private final MultisourceHttpProvider http;

  public RemoteCatalog(MultisourceHttpProvider http) {
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

  @Override
  public void queryParties(PartiesVisitor visitor) throws IOException {
    Log.d(TAG, "queryParties()");
    final InputStream stream = http.getInputStream(Uri.parse(ALL_PARTIES_QUERY));
    final RootElement root = createPartiesParserRoot(visitor);
    performQuery(stream, root);
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
    final InputStream stream = http.getInputStream(Uri.parse(query));
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(stream, root);
  }

  final boolean searchSupported() {
    return http.hasConnection();
  }

  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    Log.d(TAG, "findTracks(query=%s)", query);
    final String uri = String.format(Locale.US, FIND_TRACKS_QUERY, Uri.encode(query));
    final InputStream stream = http.getInputStream(Uri.parse(uri));
    final RootElement root = createModulesParserRoot(visitor);
    performQuery(stream, root);
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
    final Element data = result.getChild("responseData");
    data.getChild("totalAmount").setEndTextElementListener(body -> {
      final Integer count = HtmlUtils.tryGetInteger(body);
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = data.getChild("authors").getChild("author");
    item.setEndElementListener(() -> {
      final Author res = builder.captureResult();
      if (res != null) {
        visitor.accept(res);
      }
    });
    item.getChild("id").setEndTextElementListener(builder::setId);
    item.getChild("title").setEndTextElementListener(builder::setNickname);
    item.getChild("realName").setEndTextElementListener(builder::setName);
    return result;
  }

  private static class AuthorBuilder {

    @Nullable
    private Integer id;
    @Nullable
    private String nickname;
    @Nullable
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
      final Author res = isValid() ? new Author(id, nickname, name != null ? name : "") : null;
      id = null;
      nickname = null;
      name = null;
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
    data.getChild("totalAmount").setEndTextElementListener(body -> {
      final Integer count = HtmlUtils.tryGetInteger(body);
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = data.getChild("parties").getChild("party");
    item.setEndElementListener(() -> {
      final Party res = builder.captureResult();
      if (res != null) {
        visitor.accept(res);
      }
    });
    item.getChild("id").setEndTextElementListener(builder::setId);
    item.getChild("title").setEndTextElementListener(builder::setName);
    item.getChild("year").setEndTextElementListener(builder::setYear);
    return result;
  }

  private static class PartiesBuilder {

    @Nullable
    private Integer id;
    @Nullable
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
    data.getChild("totalAmount").setEndTextElementListener(body -> {
      final Integer count = HtmlUtils.tryGetInteger(body);
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = data.getChild("tunes").getChild("tune");
    item.setEndElementListener(() -> {
      final Track result1 = builder.captureResult();
      if (result1 != null) {
        visitor.accept(result1);
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
    data.getChild("totalAmount").setEndTextElementListener(body -> {
      final Integer count = HtmlUtils.tryGetInteger(body);
      if (count != null) {
        visitor.setCountHint(count);
      }
    });
    final Element item = data.getChild("tunes").getChild("tune");
    item.setEndElementListener(() -> {
      final Author author = builder.captureResultAuthor();
      final Track result1 = builder.captureResult();
      if (result1 != null && author != null) {
        visitor.accept(author, result1);
      }
    });
    bindXmlActions(item, builder);
    return result;
  }

  private static void bindXmlActions(Element item, final ModuleBuilder builder) {
    item.getChild("id").setEndTextElementListener(builder::setId);
    //CDATA
    item.getChild("originalFileName").setEndTextElementListener(body -> builder.setFilename(Html.fromHtml(body).toString()));
    item.getChild("title").setEndTextElementListener(builder::setTitle);
    item.getChild("internalAuthor").setEndTextElementListener(builder::setInternalAuthor);
    item.getChild("internalTitle").setEndTextElementListener(builder::setInternalTitle);
    item.getChild("votes").setEndTextElementListener(builder::setVotes);
    item.getChild("time").setEndTextElementListener(builder::setDuration);
    item.getChild("year").setEndTextElementListener(builder::setYear);
    item.getChild("compo").setEndTextElementListener(builder::setCompo);
    item.getChild("partyplace").setEndTextElementListener(builder::setPartyplace);
    item.getChild("authors").getChild("id").setEndTextElementListener(builder::setAuthorId);
  }

  private static class ModuleBuilder {

    @Nullable
    private Integer id;
    @Nullable
    private String filename;
    private String title = "";
    private String internalAuthor = "";
    private String internalTitle = "";
    private String votes = "";
    private String duration = "";
    private int year;
    @Nullable
    private String compo;
    private int partyplace;
    @Nullable
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
      year = 0;
      partyplace = 0;
      votes = "";
      duration = "";
      title = "";
      internalAuthor = "";
      internalTitle = "";
      compo = null;
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
