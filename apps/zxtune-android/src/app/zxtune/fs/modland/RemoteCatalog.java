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
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicReference;
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
  private final static String GROUPS_QUERY = API + "&md=b_%s&st=%s";
  private final static String GROUP_TRACKS_QUERY = API + "&md=%s&id=%d";

  private final static String STORAGE_MIRROR = "http://ftp.amigascne.org/mirrors/ftp.modland.com";

  private final static Pattern PAGINATOR =
    Pattern.compile("<caption>Browsing (.+?) - (\\d+) results? - showing page (\\d+) of (\\d+).</caption>");
  private final static Pattern TRACKS =
    Pattern.compile("file=(pub/modules/.+?).>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);

  private final HttpProvider http;
  private final Grouping authors;
  private final Grouping collections;
  private final Grouping formats;

  RemoteCatalog(Context context) {
    this.http = new HttpProvider(context);
    this.authors = new Authors();
    this.collections = new Collections();
    this.formats = new Formats();
  }

  @Override
  public Grouping getAuthors() {
    return authors;
  }

  @Override
  public Grouping getCollections() {
    return collections;
  }
  
  @Override
  public Grouping getFormats() {
    return formats;
  }

  private static String decodeHtml(String txt) {
    return Html.fromHtml(txt).toString();
  }
  
  private static String makeGroupsQuery(String type, String filter) {
    return String.format(Locale.US, GROUPS_QUERY, type, Uri.encode(filter));
  }

  private static String makeGroupTracksQuery(String type, int authorId) {
    return String.format(Locale.US, GROUP_TRACKS_QUERY, type, authorId);
  }
  
  private abstract class BaseGrouping implements Grouping {
    
    private final Pattern entries;
    
    BaseGrouping() {
      this.entries = Pattern.compile(
          "<td><a href=.+?md=" + getCategoryTag() + "&amp;id=(\\d+).>(.+?)</a>.+?<td class=.right.>(\\d+)</td>", Pattern.DOTALL);
    }
    
    @Override
    public void query(String filter, final GroupsVisitor visitor) throws IOException {
      loadPages(makeGroupsQuery(getCategoryTag(), filter), new PagesVisitor() {
        
        private boolean countReported;
        
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          if (!countReported) {
            visitor.setCountHint(results);
            countReported = true;
          }
          parseAuthors(content, visitor);
          return true;
        }
      });
    }
    
    private void parseAuthors(CharSequence content, GroupsVisitor visitor) {
      final Matcher matcher = entries.matcher(content);
      while (matcher.find()) {
        final String id = matcher.group(1);
        final String name = decodeHtml(matcher.group(2));
        final String tracks = matcher.group(3);
        visitor.accept(new Group(Integer.valueOf(id), name, Integer.valueOf(tracks)));
      }
    }

    @Override
    public Group query(final int id) throws IOException {
      final Group[] result = new Group[1];
      loadPages(makeGroupTracksQuery(getCategoryTag(), id), new PagesVisitor() {
        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          final String tracksHeader = getTracksHeader();
          if (header.startsWith(tracksHeader)) {
            final String name = header.substring(tracksHeader.length());
            result[0] = new Group(id, decodeHtml(name), results);
          }
          return false;
        }
      });
      return result[0];
    }

    @Override
    public void queryTracks(int id, final TracksVisitor visitor) throws IOException {
      loadPages(makeGroupTracksQuery(getCategoryTag(), id), new PagesVisitor() {

        private boolean countReported;

        @Override
        public boolean onPage(String header, int results, CharSequence content) {
          if (!countReported) {
            visitor.setCountHint(results);
            countReported = true;
          }
          return parseTracks(content, visitor);
        }
      });
    }
    
    @Override
    public Track findTrack(int id, final String filename) throws IOException {
      final AtomicReference<Track> result = new AtomicReference<Track>();
      queryTracks(id, new TracksVisitor() {
        @Override
        public void setCountHint(int size) {}

        @Override
        public boolean accept(Track obj) {
          if (obj.filename.equals(filename)) {
            result.set(obj);
            return false;
          } else {
            return true;
          }
        }
      });
      return result.get();
    }
    
    abstract String getTracksHeader();
    abstract String getCategoryTag();
  }
  
  private class Authors extends BaseGrouping {

    @Override
    String getTracksHeader() {
      return "Modules from author ";
    }
    
    @Override
    String getCategoryTag() {
      return "aut";
    }
  }

  private class Collections extends BaseGrouping {

    @Override
    String getTracksHeader() {
      return "Modules from collection ";
    }
    
    @Override
    String getCategoryTag() {
      return "col";
    }
  }
  
  private class Formats extends BaseGrouping {

    @Override
    String getTracksHeader() {
      return "Modules in format ";
    }
    
    @Override
    String getCategoryTag() {
      return "for";
    }
  }
  
  private static boolean parseTracks(CharSequence content, TracksVisitor visitor) {
    final Matcher matcher = TRACKS.matcher(content);
    while (matcher.find()) {
      final String path = "/" + matcher.group(1);
      final String size = matcher.group(2);
      if (!visitor.accept(new Track(path, Integer.valueOf(size)))) {
        return false;
      }
    }
    return true;
  }

  @Override
  public ByteBuffer getTrackContent(String id) throws IOException {
    return http.getContent(STORAGE_MIRROR + id);
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
