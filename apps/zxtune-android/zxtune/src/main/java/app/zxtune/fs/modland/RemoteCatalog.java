/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland;

import android.net.Uri;

import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.Log;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.api.Cdn;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.utils.ProgressCallback;
import app.zxtune.utils.StubProgressCallback;

/**
 * There's additional api gate for xbmc plugin at
 * http://www.exotica.org.uk/mediawiki/extensions/ExoticASearch/Modland_xbmc.php
 * but it seems to be not working and has no such wide catalogue as http gate does.
 */
public class RemoteCatalog implements Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private static final Pattern PAGINATOR =
          Pattern.compile("Browsing (.+?) - (\\d+) results? - showing page (\\d+) of (\\d+).");

  private final MultisourceHttpProvider http;
  private final Grouping authors;
  private final Grouping collections;
  private final Grouping formats;

  RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
    this.authors = new BaseGrouping("aut", "Modules from author ");
    this.collections = new BaseGrouping("col", "Modules from collection ");
    this.formats = new BaseGrouping("for", "Modules in format ");
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

  private static Uri.Builder getMainUriBuilder() {
    return new Uri.Builder().scheme("https").authority("www.exotica.org.uk").path("mediawiki" +
        "/index.php").appendQueryParameter("title", "Special:Modland");
  }

  private static Uri.Builder getGroupsUriBuilder(String type, String filter) {
    return getMainUriBuilder().appendQueryParameter("md", "b_" + type).appendQueryParameter("st",
        filter);
  }

  private static Uri.Builder getTracksUriBuilder(String type, int id) {
    return getMainUriBuilder().appendQueryParameter("md", type).appendQueryParameter("id",
        Integer.toString(id));
  }

  private class BaseGrouping implements Grouping {

    private final String tag;
    private final String headerPrefix;

    BaseGrouping(String tag, String headerPrefix) {
      this.tag = tag;
      this.headerPrefix = headerPrefix;
    }

    @Override
    public void queryGroups(String filter, final GroupsVisitor visitor,
                            final ProgressCallback progress) throws IOException {
      Log.d(TAG, "queryGroups(type=%s, filter=%s)", tag, filter);
      loadPages(getGroupsUriBuilder(tag, filter), new PagesVisitor() {

        private int total = 0;
        private int done = 0;

        @Override
        public boolean onPage(String header, int results, Document doc) {
          if (total == 0) {
            total = results;
            visitor.setCountHint(total);
          }
          done += parseGroups(doc, visitor);
          progress.onProgressUpdate(done, total);
          return true;
        }
      });
    }

    @Override
    public Group getGroup(final int id) throws IOException {
      Log.d(TAG, "getGroup(type=%s, id=%d)", tag, id);
      final Group[] resultRef = new Group[1];
      loadPages(getTracksUriBuilder(tag, id), (header, results, doc) -> {
        if (header.startsWith(headerPrefix)) {
          final String name = header.substring(headerPrefix.length());
          resultRef[0] = new Group(id, name, results);
        }
        return false;
      });
      final Group result = resultRef[0];
      if (result != null) {
        return result;
      }
      throw new IOException(String.format(Locale.US,"Failed to find group with id=%d", id));
    }

    @Override
    public void queryTracks(int id, final TracksVisitor visitor, final ProgressCallback progress) throws IOException {
      Log.d(TAG, "queryGroupTracks(type=%s, id=%d)", tag, id);
      loadPages(getTracksUriBuilder(tag, id), new PagesVisitor() {

        private int total = 0;
        private int done = 0;

        @Override
        public boolean onPage(String header, int results, Document doc) {
          if (total == 0) {
            total = results;
            visitor.setCountHint(total);
          }
          final int tracks = parseTracks(doc, visitor);
          done += tracks;
          progress.onProgressUpdate(done, total);
          return tracks != 0;
        }
      });
    }

    @Override
    public Track getTrack(int id, final String filename) throws IOException {
      Log.d(TAG, "getGroupTrack(type=%s, id=%d, filename=%s)", tag, id, filename);
      final Track[] resultRef = {null};
      queryTracks(id, new TracksVisitor() {
        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public boolean accept(Track obj) {
          if (obj.filename.equals(filename)) {
            resultRef[0] = obj;
            return false;
          } else {
            return true;
          }
        }
      }, StubProgressCallback.instance());
      final Track result = resultRef[0];
      if (result != null) {
        return result;
      }
      throw new IOException(String.format(Locale.US, "Failed to get track '%s' with id=%d", filename, id));
    }
  }

  private static int parseGroups(Document doc, GroupsVisitor visitor) {
    int result = 0;
    for (Element el : doc.select("table:has(>caption)>tbody>tr:has(>td>a[href*=md=])")) {
      final Element nameEl = el.child(0).child(0);
      final Element countEl = el.child(1);
      final Integer id = HtmlUtils.getQueryInt(nameEl, "id");
      final String name = nameEl.text();
      final Integer tracks = HtmlUtils.tryGetInteger(countEl.text());
      if (id != null && name != null && tracks != null) {
        visitor.accept(new Group(id, name, tracks));
        ++result;
      }
    }
    return result;
  }

  private static int parseTracks(Document doc, TracksVisitor visitor) {
    int result = 0;
    for (Element el : doc.select("table:has(>caption)>tbody>tr:has(>td>a[href*=file=pub/])")) {
      final Element pathEl = el.child(0).child(0);
      final Element sizeEl = el.child(4);
      final String href = pathEl.attr("href");
      final int pathPos = href.indexOf("pub/modules/");
      final String path = "/" + href.substring(pathPos);
      final Integer size = HtmlUtils.tryGetInteger(sizeEl.text());
      if (size != null) {
        if (!visitor.accept(new Track(path, size))) {
          return 0;
        }
        ++result;
      }
    }
    return result;
  }

  public static Uri[] getTrackUris(String id) {
    return new Uri[]{
        Cdn.modland(id),
        Uri.parse("http://ftp.amigascne.org/mirrors/ftp.modland.com" + id)
    };
  }

  interface PagesVisitor {
    boolean onPage(String title, int results, Document doc);
  }

  private void loadPages(Uri.Builder query, PagesVisitor visitor) throws IOException {
    for (int pg = 1; ; ++pg) {
      final Document doc = HtmlUtils.parseDoc(readPage(query, pg));
      final Element hdr = doc.selectFirst("table>caption");
      final Matcher matcher = hdr != null ? PAGINATOR.matcher(hdr.text()) : null;
      if (matcher != null && matcher.find()) {
        Log.d(TAG, "Load page: " + hdr.text());
        final String header = matcher.group(1);
        final String results = matcher.group(2);
        final String page = matcher.group(3);
        final String pagesTotal = matcher.group(4);
        if (pg != Integer.parseInt(page)) {
          throw new IOException("Invalid paginator structure");
        }
        if (visitor.onPage(header, Integer.parseInt(results), doc)
                && pg < Integer.parseInt(pagesTotal)) {
          continue;
        }
      }
      break;
    }
  }

  private InputStream readPage(Uri.Builder base, int start) throws IOException {
    final Uri uri = base.appendQueryParameter("pg", Integer.toString(start)).build();
    return http.getInputStream(uri);
  }
}
