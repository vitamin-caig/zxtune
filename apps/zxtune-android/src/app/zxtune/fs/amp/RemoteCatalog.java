/**
 *
 * @file
 *
 * @brief Remote implementation of catalog
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;
import android.text.Html;
import app.zxtune.Log;
import app.zxtune.fs.HttpProvider;

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
 */

class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private final static String SITE = "http://amp.dascene.net/";
  
  private final static String GROUPS_URI = SITE + "newresult.php?request=groups";
  private final static String BY_HANDLE_URI_FORMAT = SITE + "newresult.php?request=list&search=%s";
  private final static String BY_COUNTRY_URI_FORMAT = SITE + "newresult.php?request=country&search=%s";
  private final static String BY_GROUP_URI_FORMAT = SITE + "newresult.php?request=groupid&search=%s";
  //private final static String AUTHOR_URI_FORMAT = SITE + "detail.php?view=%d";
  private final static String AUTHOR_TRACKS_URI_FORMAT = SITE + "detail.php?detail=modules&view=%d";
  private final static String TRACK_URI_FORMAT = SITE + "downmod.php?index=%d";

  private final static Pattern PAGINATOR =
    Pattern.compile("<caption>.+?" +
                    "(<a href=.+?position=([0-9]+).+?left.gif.+?)?" +
                    "(<a href=.+?position=([0-9]+).+?right.gif.+?)?" +
                    "</caption>", Pattern.DOTALL);
  private final static Pattern GROUPS =
    Pattern.compile("<a href=.newresult.php.request=groupid.search=([0-9]+).>(.+?)</a>", Pattern.DOTALL);
  private final static Pattern AUTHORS = 
    Pattern.compile("Handle:.+?<a href=.detail.php.view=([0-9]+).+?>(.+?)</a>.+?" +
                    "Real Name:.+?<td>(.+?)</td>", Pattern.DOTALL);
  /*
  private final static Pattern AUTHOR =
      Pattern.compile("Handle:.+?<td>(.+?)\\s+?</td>.+?" +
          "Real.+?Name:.+?<td>(.+?)\\s+?</td>", Pattern.DOTALL);
  */
  private final static Pattern TRACKS =
    Pattern.compile("<a href=.downmod.php.index=([0-9]+).+?>(.+?)</a>.+?" +
                    "<td>([0-9]+)Kb</td>", Pattern.DOTALL);

  private final HttpProvider http;

  RemoteCatalog(Context context) {
    this.http = new HttpProvider(context);
  }

  private static String decodeHtml(String txt) {
    return Html.fromHtml(txt).toString();
  }

  @Override
  public void queryGroups(GroupsVisitor visitor) throws IOException {
    final String content = http.getHtml(GROUPS_URI);
    final Matcher matcher = GROUPS.matcher(content);
    while (matcher.find()) {
      final String id = matcher.group(1);
      final String name = matcher.group(2);
      visitor.accept(new Group(Integer.valueOf(id), decodeHtml(name)));
    }
  }
  
  @Override
  public void queryAuthors(String handleFilter, AuthorsVisitor visitor) throws IOException {
    final String uri = String.format(Locale.US, BY_HANDLE_URI_FORMAT, handleFilter);
    queryAuthorsInternal(uri, visitor);
  }
  
  @Override
  public void queryAuthors(Country country, AuthorsVisitor visitor) throws IOException {
    final String uri = String.format(Locale.US, BY_COUNTRY_URI_FORMAT, country.id);
    queryAuthorsInternal(uri, visitor);
  }

  @Override
  public void queryAuthors(Group group, AuthorsVisitor visitor) throws IOException {
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
    final String uri = String.format(Locale.US, AUTHOR_TRACKS_URI_FORMAT, author.id);
    final String content = http.getHtml(uri);
    final Matcher matcher = TRACKS.matcher(content);
    while (matcher.find()) {
      final Integer id = Integer.valueOf(matcher.group(1));
      final String name = decodeHtml(matcher.group(2));
      final Integer size = Integer.valueOf(matcher.group(3));
      visitor.accept(new Track(id, name, size));
    }
  }

  @Override
  public ByteBuffer getTrackContent(int id) throws IOException {
    final String uri = String.format(Locale.US, TRACK_URI_FORMAT, id);
    return http.getContent(uri);
  }

  interface PagesVisitor {
    boolean onPage(CharSequence content);
  }
  
  private void loadPages(String query, PagesVisitor visitor) throws IOException {
    for (int offset = 0; ; ) {
      final String uri = query + String.format(Locale.US, "&position=%d", offset);
      final String chars = http.getHtml(uri);
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
