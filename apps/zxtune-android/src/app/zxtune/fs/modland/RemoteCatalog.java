/**
 *
 * @file
 *
 * @brief Remote implementation of catalog
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import java.io.IOException;
import java.lang.ref.Reference;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.fs.HttpProvider;

/**
 * Use pure http response parsing via regex in despite that page structure seems to be xml well formed.
 *
 * There's additional api gate for xbmc plugin at
 * http://www.exotica.org.uk/mediawiki/extensions/ExoticASearch/Modland_xbmc.php
 * but it seems to be not working and has no such wide catalogue as http gate does.
 */
class RemoteCatalog extends Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private final static String SITE = "http://www.exotica.org.uk/";
  private final static String API = SITE + "mediawiki/index.php?title=Special:Modland";
  private final static String AUTHORS_QUERY = API + "&md=b_aut&st=%s";
  private final static String AUTHOR_TRACKS_QUERY = API + "&md=aut&id=%d";

  private final static String STORAGE_MIRROR = "http://ftp.amigascne.org/mirrors/ftp.modland.com";

  private final Pattern PAGINATOR =
    Pattern.compile("<caption>Browsing (.+?) - (\\d+) results? - showing page (\\d+) of (\\d+).</caption>");
  private final Pattern AUTHORS =
    Pattern.compile("<td><a href=.+?md=aut&amp;id=(\\d+).>(.+?)</a>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);
  private final Pattern TRACKS =
    Pattern.compile("file=(pub/modules/.+?).>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);
  private final String AUTHOR_TRACKS_HEADER = "Modules from author ";

  private final HttpProvider http;

  RemoteCatalog(Context context) {
    this.http = new HttpProvider(context);
  }

  @Override
  public void queryAuthors(String filter, final AuthorsVisitor visitor) throws IOException {
    loadPages(makeAuthorsQuery(filter), new PagesVisitor() {
      @Override
      public boolean onPage(String title, int results, CharSequence content) {
        //title = 'authors starting with ${filter}'
        parseAuthors(content, visitor);
        return true;
      }
    });
  }

  private void parseAuthors(CharSequence content, AuthorsVisitor visitor) {
    final Matcher matcher = AUTHORS.matcher(content);
    while (matcher.find()) {
      final String id = matcher.group(1);
      final String nick = matcher.group(2);
      final String tracks = matcher.group(3);
      visitor.accept(new Author(Integer.valueOf(id), nick, Integer.valueOf(tracks)));
    }
  }

  @Override
  public Author queryAuthor(final int id) throws IOException {
    final Author[] result = new Author[1];
    loadPages(makeAuthorTracksQuery(id), new PagesVisitor() {
      @Override
      public boolean onPage(String title, int results, CharSequence content) {
        //title = Modules from author ${nick}
        if (title.startsWith(AUTHOR_TRACKS_HEADER)) {
          result[0] = new Author(id, title.substring(AUTHOR_TRACKS_HEADER.length()), results);
        }
        return false;
      }
    });
    return result[0];
  }

  @Override
  public void queryAuthorTracks(int authorId, final TracksVisitor visitor) throws IOException {
    loadPages(makeAuthorTracksQuery(authorId), new PagesVisitor() {
      @Override
      public boolean onPage(String title, int results, CharSequence content) {
        //title = Modules from author ${nick}
        parseTracks(content, visitor);
        return true;
      }
    });
  }

  private void parseTracks(CharSequence content, TracksVisitor visitor) {
    final Matcher matcher = TRACKS.matcher(content);
    while (matcher.find()) {
      final String path = "/" + matcher.group(1);
      final String size = matcher.group(2);
      visitor.accept(new Track(path, Integer.valueOf(size)));
    }
  }

  @Override
  public ByteBuffer getTrackContent(String id) throws IOException {
    return http.getContent(STORAGE_MIRROR + id);
  }

  private String makeAuthorsQuery(String filter) {
    return String.format(Locale.US, AUTHORS_QUERY, Uri.encode(filter));
  }

  private String makeAuthorTracksQuery(int authorId) {
    return String.format(Locale.US, AUTHOR_TRACKS_QUERY, authorId);
  }

  interface PagesVisitor {
    boolean onPage(String title, int results, CharSequence content);
  }

  private void loadPages(String query, PagesVisitor visitor) throws IOException {
    for (int pg = 1; ; ++pg) {
      final String uri = query + String.format(Locale.US, "&pg=%d", pg);
      final ByteBuffer buf = http.getContent(uri);
      final String chars = new String(buf.array(), "UTF-8");
      final Matcher matcher = PAGINATOR.matcher(chars);
      if (matcher.find()) {
        Log.d(TAG, "Load page: " + matcher.group());
        final String subtitle = matcher.group(1);
        final String results = matcher.group(2);
        final String page = matcher.group(3);
        final String pagesTotal = matcher.group(4);
        if (pg != Integer.valueOf(page)) {
          throw new UnsupportedOperationException("Invalid paginator structure");
        }
        if (visitor.onPage(subtitle, Integer.valueOf(results), chars)
            && pg < Integer.valueOf(pagesTotal)) {
          continue;
        }
      }
      break;
    }
  }
}
