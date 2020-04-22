/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.net.Uri;
import android.text.Html;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.Log;
import app.zxtune.fs.api.Cdn;
import app.zxtune.fs.http.HttpObject;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.io.Io;

/**
 * Authors:
 *   http://amp.dascene.net/newresult.php?request=list&search=${letter}&position=${offset} letter='0-9',a..z
 *
 * references to author's info at
 *   http://amp.dascene.net/detail.php?view=${authorid}
 *
 * references to author's track at
 *   http://amp.dascene.net/detail.php?detail=modules&view=${authorid}
 *
 * references tracks at
 *   http://amp.dascene.net/downmod.php?index=${trackid}
 *
 * Countries:
 *   http://amp.dascene.net/newresult.php?request=country&search=${countryid} id=1..64
 *
 * Groups:
 *   http://amp.dascene.net/newresult.php?request=groupid&search=${groupid} id=1..7149
 *
 * list all:
 *   http://amp.dascene.net/newresult.php?request=groups
 *
 * Search:
 *   http://amp.dascene.net/newresult.php?request=module&search=${whatever}
 */

class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static final String SITE = "http://amp.dascene.net/";

  private static final String GROUPS_URI = SITE + "newresult.php?request=groups";
  private static final String BY_HANDLE_URI_FORMAT = SITE + "newresult.php?request=list&search=%s";
  private static final String BY_COUNTRY_URI_FORMAT = SITE + "newresult.php?request=country&search=%s";
  private static final String BY_GROUP_URI_FORMAT = SITE + "newresult.php?request=groupid&search=%s";
  //private final static String AUTHOR_URI_FORMAT = SITE + "detail.php?view=%d";
  private static final String AUTHOR_TRACKS_URI_FORMAT = SITE + "detail.php?detail=modules&view=%d";
  private static final String FIND_TRACK_URI_FORMAT = SITE + "newresult.php?request=module&search=%s";

  private static final Pattern PAGINATOR =
          Pattern.compile("<caption>.+?" +
                  "(<a href=.+?position=([0-9]+).+?left.gif.+?)?" +
                  "(<a href=.+?position=([0-9]+).+?right.gif.+?)?" +
                  "</caption>", Pattern.DOTALL);
  private static final Pattern GROUPS =
          Pattern.compile("<a href=.newresult.php.request=groupid.search=([0-9]+).>(.+?)</a>", Pattern.DOTALL);

  private static final String AUTHOR_ANCHOR = "<a href=.detail.php.view=([0-9]+).+?>(.+?)</a>.+?";

  private static final Pattern AUTHORS =
          Pattern.compile("Handle:.+?" + AUTHOR_ANCHOR +
                  "Real Name:.+?<td>(.+?)</td>", Pattern.DOTALL);
  /*
  private final static Pattern AUTHOR =
      Pattern.compile("Handle:.+?<td>(.+?)\\s+?</td>.+?" +
          "Real.+?Name:.+?<td>(.+?)\\s+?</td>", Pattern.DOTALL);
  */

  private static final String TRACK_ANCHOR = "<a href=.downmod.php.index=([0-9]+).+?>(.+?)</a>.+?";
  private static final String TRACK_SIZE = "<td>([0-9]+)Kb</td>";

  private static final Pattern TRACKS =
          Pattern.compile(TRACK_ANCHOR +
                  TRACK_SIZE, Pattern.DOTALL);

  private static final Pattern FOUND_TRACKS =
          Pattern.compile(TRACK_ANCHOR +
                  AUTHOR_ANCHOR +
                  TRACK_SIZE, Pattern.DOTALL);

  private final HttpProvider http;
  private final MultisourceHttpProvider multiHttp;

  RemoteCatalog(HttpProvider http) {
    this.http = http;
    this.multiHttp = new MultisourceHttpProvider(http);
  }

  private static String decodeHtml(String txt) {
    return Html.fromHtml(txt).toString();
  }

  @Override
  public void queryGroups(GroupsVisitor visitor) throws IOException {
    Log.d(TAG, "queryGroups()");
    final String content = Io.readHtml(http.getInputStream(Uri.parse(GROUPS_URI)));
    final Matcher matcher = GROUPS.matcher(content);
    while (matcher.find()) {
      final String id = matcher.group(1);
      final String name = matcher.group(2);
      visitor.accept(new Group(Integer.valueOf(id), decodeHtml(name)));
    }
  }

  @Override
  public void queryAuthors(String handleFilter, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(handleFilter=%s)", handleFilter);
    final String uri = String.format(Locale.US, BY_HANDLE_URI_FORMAT, handleFilter);
    queryAuthorsInternal(uri, visitor);
  }

  @Override
  public void queryAuthors(Country country, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(country=%d)", country.id);
    final String uri = String.format(Locale.US, BY_COUNTRY_URI_FORMAT, country.id);
    queryAuthorsInternal(uri, visitor);
  }

  @Override
  public void queryAuthors(Group group, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(group=%d)", group.id);
    final String uri = String.format(Locale.US, BY_GROUP_URI_FORMAT, group.id);
    queryAuthorsInternal(uri, visitor);
  }

  private void queryAuthorsInternal(String uri, final AuthorsVisitor visitor) throws IOException {
    loadPages(uri, new PagesVisitor() {
      @Override
      public boolean onPage(CharSequence content) {
        parseAuthors(content, visitor);
        return true;
      }
    });
  }

  private static void parseAuthors(CharSequence content, AuthorsVisitor visitor) {
    final Matcher matcher = AUTHORS.matcher(content);
    while (matcher.find()) {
      final String id = matcher.group(1);
      final String name = matcher.group(2);
      final String realName = matcher.group(3);
      visitor.accept(new Author(Integer.valueOf(id), decodeHtml(name), decodeHtml(realName)));
    }
  }

  @Override
  public void queryTracks(Author author, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryTracks(author=%d)", author.id);
    final String uri = String.format(Locale.US, AUTHOR_TRACKS_URI_FORMAT, author.id);
    final String content = Io.readHtml(http.getInputStream(Uri.parse(uri)));
    final Matcher matcher = TRACKS.matcher(content);
    while (matcher.find()) {
      final Integer id = Integer.valueOf(matcher.group(1));
      final String name = decodeHtml(matcher.group(2));
      final Integer size = Integer.valueOf(matcher.group(3));
      visitor.accept(new Track(id, name, size));
    }
  }

  final boolean searchSupported() {
    return http.hasConnection();
  }

  @Override
  public void findTracks(String query, final FoundTracksVisitor visitor) throws IOException {
    Log.d(TAG, "findTracks(query=%s)", query);
    final String uri = String.format(Locale.US, FIND_TRACK_URI_FORMAT, Uri.encode(query));
    loadPages(uri, new PagesVisitor() {
      @Override
      public boolean onPage(CharSequence content) {
        parseFoundTracks(content, visitor);
        return true;
      }
    });
  }

  private static void parseFoundTracks(CharSequence content, FoundTracksVisitor visitor) {
    final Matcher matcher = FOUND_TRACKS.matcher(content);
    while (matcher.find()) {
      final Integer trackId = Integer.valueOf(matcher.group(1));
      final String trackName = decodeHtml(matcher.group(2));
      final Integer authorId = Integer.valueOf(matcher.group(3));
      final String authorHandle = decodeHtml(matcher.group(4));
      final Integer size = Integer.valueOf(matcher.group(5));
      visitor.accept(new Author(authorId, authorHandle, ""/*realName*/), new Track(trackId, trackName, size));
    }
  }

  @Override
  @NonNull
  public ByteBuffer getTrackContent(int id) throws IOException {
    Log.d(TAG, "getTrackContent(%d)", id);
    return Io.readFrom(multiHttp.getInputStream(getContentUris(id)));
  }

  final HttpObject getTrackObject(int id) throws IOException {
    Log.d(TAG, "getTrackObject(%d)", id);
    return multiHttp.getObject(getContentUris(id));
  }

  private static Uri[] getContentUris(int id) {
    return new Uri[]{
      Cdn.amp(id),
      getMainUriBuilder().appendPath("downmod.php").appendQueryParameter("index", Integer.toString(id)).build()
    };
  }

  private static Uri.Builder getMainUriBuilder() {
    return new Uri.Builder().scheme("http").authority("amp.dascene.net");
  }

  interface PagesVisitor {
    boolean onPage(CharSequence content);
  }

  private void loadPages(String query, PagesVisitor visitor) throws IOException {
    for (int offset = 0; ; ) {
      final String uri = query + String.format(Locale.US, "&position=%d", offset);
      final String chars = Io.readHtml(http.getInputStream(Uri.parse(uri)));
      final Matcher matcher = PAGINATOR.matcher(chars);
      if (matcher.find()) {
        Log.d(TAG, "Load page: %s", uri);
        final String nextPageOffset = matcher.group(4);
        final String next = nextPageOffset != null
                ? nextPageOffset : "";
        if (visitor.onPage(chars) && !next.isEmpty() && !next.equals(uri)) {
          offset = Integer.valueOf(next);
          continue;
        }
      }
      break;
    }
  }
}
