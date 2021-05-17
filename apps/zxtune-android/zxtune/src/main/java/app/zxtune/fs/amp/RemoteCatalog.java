/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.net.Uri;

import androidx.annotation.Nullable;

import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.IOException;
import java.io.InputStream;

import app.zxtune.Log;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.api.Cdn;
import app.zxtune.fs.http.MultisourceHttpProvider;

/**
 * Authors:
 * http://amp.dascene.net/newresult.php?request=list&search=${letter}&position=${offset} letter='0-9',a..z
 * <p>
 * references to author's info at
 * http://amp.dascene.net/detail.php?view=${authorid}
 * <p>
 * references to author's track at
 * http://amp.dascene.net/detail.php?detail=modules&view=${authorid}
 * <p>
 * references tracks at
 * http://amp.dascene.net/downmod.php?index=${trackid}
 * <p>
 * Countries:
 * http://amp.dascene.net/newresult.php?request=country&search=${countryid} id=1..64
 * <p>
 * Groups:
 * http://amp.dascene.net/newresult.php?request=groupid&search=${groupid} id=1..7149
 * <p>
 * list all:
 * http://amp.dascene.net/newresult.php?request=groups
 * <p>
 * Search:
 * http://amp.dascene.net/newresult.php?request=module&search=${whatever}
 */

public class RemoteCatalog implements Catalog {

  private static final String TAG = RemoteCatalog.class.getName();

  private final MultisourceHttpProvider http;

  RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
  }

  @Override
  public void queryGroups(GroupsVisitor visitor) throws IOException {
    Log.d(TAG, "queryGroups()");
    final Document doc =
        HtmlUtils.parseDoc(http.getInputStream(getQueryUriBuilder("groups", "").build()));
    for (Element el : doc.select("tr>td>a[href^=newresult.php?request=groupid]")) {
      final Integer id = HtmlUtils.getQueryInt(el, "search");
      final String name = el.text();
      if (id != null && name != null) {
        visitor.accept(new Group(id, name));
      }
    }
  }

  @Override
  public void queryAuthors(String handleFilter, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(handleFilter=%s)", handleFilter);
    queryAuthorsInternal(getQueryUriBuilder("list", handleFilter), visitor);
  }

  @Override
  public void queryAuthors(Country country, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(country=%d)", country.id);
    queryAuthorsInternal(getQueryUriBuilder("country", Integer.toString(country.id)), visitor);
  }

  @Override
  public void queryAuthors(Group group, AuthorsVisitor visitor) throws IOException {
    Log.d(TAG, "queryAuthors(group=%d)", group.id);
    queryAuthorsInternal(getQueryUriBuilder("groupid", Integer.toString(group.id)), visitor);
  }

  private void queryAuthorsInternal(Uri.Builder uri, final AuthorsVisitor visitor) throws IOException {
    loadPages(uri, doc -> {
      parseAuthors(doc, visitor);
      return true;
    });
  }

  private static void parseAuthors(Document doc, AuthorsVisitor visitor) {
    for (Element el : doc.select("table:has(>caption)>tbody>tr>td>table>tbody")) {
      final Element handleEl = el.selectFirst("a[href^=detail.php]");
      final Element realNameEl = el.selectFirst("td:containsOwn(Real Name) + td");
      final Integer id = HtmlUtils.getQueryInt(handleEl, "view");
      final String name = handleEl.text();
      final String realName = realNameEl.text();
      if (id != null && name != null && realName != null) {
        visitor.accept(new Author(id, name, realName));
      }
    }
  }

  @Override
  public void queryTracks(Author author, TracksVisitor visitor) throws IOException {
    Log.d(TAG, "queryTracks(author=%d)", author.id);
    final Document doc = HtmlUtils.parseDoc(http.getInputStream(getAuthorTracksUri(author.id)));
    for (Element el : doc.select("table>tbody>tr:has(>td>a[href^=downmod.php])")) {
      final Element nameEl = el.child(0).child(0);
      final Element sizeEl = el.child(3);
      final Integer id = HtmlUtils.getQueryInt(nameEl, "index");
      final String name = nameEl.text();
      final Integer size = getSize(sizeEl.text());
      if (id != null && name != null && size != null) {
        visitor.accept(new Track(id, name, size));
      }
    }
  }

  final boolean searchSupported() {
    return http.hasConnection();
  }

  @Override
  public void findTracks(String query, final FoundTracksVisitor visitor) throws IOException {
    Log.d(TAG, "findTracks(query=%s)", query);
    final Uri.Builder uri = getQueryUriBuilder("module", query);
    loadPages(uri, doc -> {
      parseFoundTracks(doc, visitor);
      return true;
    });
  }

  private static void parseFoundTracks(Document doc, FoundTracksVisitor visitor) {
    for (Element el : doc.select("table:has(>caption)>tbody>tr:has(>td>a[href^=downmod.php])")) {
      final Element nameEl = el.child(0).child(0);
      final Element authorEl = el.child(1).child(0);
      final Element sizeEl = el.child(3);
      final Integer trackId = HtmlUtils.getQueryInt(nameEl, "index");
      final String trackName = nameEl.text();
      final Integer authorId = HtmlUtils.getQueryInt(authorEl, "view");
      final String authorHandle = authorEl.text();
      final Integer size = getSize(sizeEl.text());
      if (trackId != null && trackName != null && authorId != null && authorHandle != null && size != null) {
        visitor.accept(new Author(authorId, authorHandle, ""), new Track(trackId, trackName, size));
      }
    }
  }

  public static Uri[] getTrackUris(int id) {
    return new Uri[]{
        Cdn.amp(id),
        getMainUriBuilder().appendPath("downmod.php").appendQueryParameter("index", Integer.toString(id)).build()
    };
  }

  private static Uri.Builder getMainUriBuilder() {
    return new Uri.Builder().scheme("https").authority("amp.dascene.net");
  }

  private static Uri.Builder getQueryUriBuilder(String request, String search) {
    return getMainUriBuilder().appendPath("newresult.php")
        .appendQueryParameter("request", request)
        .appendQueryParameter(
            "search", search);
  }

  private static Uri getAuthorTracksUri(int id) {
    return getMainUriBuilder().appendPath("detail.php").appendQueryParameter("detail", "modules").appendQueryParameter("view", Integer.toString(id)).build();
  }

  @Nullable
  private static Integer getSize(String sizeKb) {
    final String raw = sizeKb.endsWith("Kb") ? sizeKb.substring(0, sizeKb.length() - 2) : null;
    return HtmlUtils.tryGetInteger(raw);
  }

  interface PagesVisitor {
    boolean onPage(Document doc);
  }

  private void loadPages(Uri.Builder query, PagesVisitor visitor) throws IOException {
    for (int offset = 0; ; ) {
      final Document doc = HtmlUtils.parseDoc(readPage(query, offset));
      final Element paginator = doc.selectFirst("table>caption");
      if (paginator == null || paginator.childrenSize() > 2) {
        break;
      }
      final Element nextPage = paginator.selectFirst("a[href^=newresult" +
          ".php?request=]:has(img[src$=/right.gif])");
      final Integer next = nextPage != null ?
          HtmlUtils.getQueryInt(nextPage, "position") : null;
      if (visitor.onPage(doc) && next != null && next != offset) {
        offset = next;
        continue;
      }
      break;
    }
  }

  private InputStream readPage(Uri.Builder base, int start) throws IOException {
    final Uri uri = base.appendQueryParameter("position", Integer.toString(start)).build();
    Log.d(TAG, "Load page: " + uri);
    return http.getInputStream(uri);
  }
}
