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
import android.text.Html;
import android.util.Log;

import java.io.IOException;
import java.lang.ref.Reference;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.fs.HttpProvider;
import app.zxtune.fs.zxtunes.Author;

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
  private final static String GROUPS_QUERY = API + "&md=%s&st=%s";
  private final static String GROUP_TRACKS_QUERY = API + "&md=%s&id=%d";

  private final static String STORAGE_MIRROR = "http://ftp.amigascne.org/mirrors/ftp.modland.com";

  private final Pattern PAGINATOR =
    Pattern.compile("<caption>Browsing (.+?) - (\\d+) results? - showing page (\\d+) of (\\d+).</caption>");
  private final Pattern TRACKS =
    Pattern.compile("file=(pub/modules/.+?).>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);

  private final HttpProvider http;
  private final Grouping authors;
  private final Grouping collections;

  RemoteCatalog(Context context) {
    this.http = new HttpProvider(context);
    this.authors = new Authors();
    this.collections = new Collections();
  }

  @Override
  public Grouping getAuthors() {
    return authors;
  }

  @Override
  public Grouping getCollections() {
    return collections;
  }

  static String decodeHtml(String txt) {
    return Html.fromHtml(txt).toString();
  }
  
  private class Authors implements Grouping {

    private final String TRACKS_HEADER = "Modules from author ";
    private final Pattern ENTRIES =
      Pattern.compile("<td><a href=.+?md=aut&amp;id=(\\d+).>(.+?)</a>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);

    @Override
    public void query(String filter, final GroupsVisitor visitor) throws IOException {
      loadPages(makeGroupsQuery("b_aut", filter), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          //header = 'authors starting with ${filter}'
          parseAuthors(content, visitor);
          return true;
        }
      });
    }
    
    private void parseAuthors(CharSequence content, GroupsVisitor visitor) {
      final Matcher matcher = ENTRIES.matcher(content);
      while (matcher.find()) {
        final String id = matcher.group(1);
        final String nick = decodeHtml(matcher.group(2));
        final String tracks = matcher.group(3);
        visitor.accept(new Group(Integer.valueOf(id), nick, Integer.valueOf(tracks)));
      }
    }

    @Override
    public Group query(final int id) throws IOException {
      final Group[] result = new Group[1];
      loadPages(makeGroupTracksQuery("aut", id), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          //header = Modules from author ${nick}
          if (header.startsWith(TRACKS_HEADER)) {
            final String nick = header.substring(TRACKS_HEADER.length());
            result[0] = new Group(id, decodeHtml(nick), results);
          }
          return false;
        }
      });
      return result[0];
    }

    @Override
    public void queryTracks(int id, final TracksVisitor visitor) throws IOException {
      loadPages(makeGroupTracksQuery("aut", id), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          //header = Modules from author ${nick}
          parseTracks(content, visitor);
          return true;
        }
      });
    }
  }

  private class Collections implements Grouping {

    private final String TRACKS_HEADER = "Modules from collection ";
    private final Pattern ENTRIES =
      Pattern.compile("<td><a href=.+?md=col&amp;id=(\\d+).>(.+?)</a>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);

    @Override
    public void query(String filter, final GroupsVisitor visitor) throws IOException {
      loadPages(makeGroupsQuery("b_col", filter), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          //header = 'collections starting with ${filter}'
          parseCollections(content, visitor);
          return true;
        }
      });
    }

    private void parseCollections(CharSequence content, GroupsVisitor visitor) {
      final Matcher matcher = ENTRIES.matcher(content);
      while (matcher.find()) {
        final String id = matcher.group(1);
        final String title = decodeHtml(matcher.group(2));
        final String tracks = matcher.group(3);
        visitor.accept(new Group(Integer.valueOf(id), title, Integer.valueOf(tracks)));
      }
    }

    @Override
    public Group query(final int id) throws IOException {
      final Group[] result = new Group[1];
      loadPages(makeGroupTracksQuery("col", id), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
        //title = Modules from collection ${title}
        if (header.startsWith(TRACKS_HEADER)) {
          final String title = header.substring(TRACKS_HEADER.length());
          result[0] = new Group(id, decodeHtml(title), results);
        }
        return false;
        }
      });
      return result[0];
    }

    @Override
    public void queryTracks(int id, final TracksVisitor visitor) throws IOException {
      loadPages(makeGroupTracksQuery("col", id), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          //header = Modules from collection ${title}
          parseTracks(content, visitor);
          return true;
        }
      });
    }
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

  private String makeGroupsQuery(String type, String filter) {
    return String.format(Locale.US, GROUPS_QUERY, type, Uri.encode(filter));
  }

  private String makeGroupTracksQuery(String type, int authorId) {
    return String.format(Locale.US, GROUP_TRACKS_QUERY, type, authorId);
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
        final String header = matcher.group(1);
        final String results = matcher.group(2);
        final String page = matcher.group(3);
        final String pagesTotal = matcher.group(4);
        if (pg != Integer.valueOf(page)) {
          throw new UnsupportedOperationException("Invalid paginator structure");
        }
        if (visitor.onPage(header, Integer.valueOf(results), chars)
            && pg < Integer.valueOf(pagesTotal)) {
          continue;
        }
      }
      break;
    }
  }
}
