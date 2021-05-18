package app.zxtune.fs.vgmrips;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.Nullable;
import androidx.core.util.Pair;

import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.nodes.TextNode;
import org.jsoup.select.Elements;

import java.io.IOException;

import app.zxtune.TimeStamp;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.api.Cdn;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.utils.ProgressCallback;

public class RemoteCatalog implements Catalog {

  private final MultisourceHttpProvider http;
  private final Grouping compaines;
  private final Grouping composers;
  private final Grouping chips;
  private final Grouping systems;

  RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
    this.compaines = new CompaniesGrouping();
    this.composers = new ComposersGrouping();
    this.chips = new ChipsGrouping();
    this.systems = new SystemsGrouping();
  }

  @Override
  public Grouping companies() {
    return compaines;
  }

  @Override
  public Grouping composers() {
    return composers;
  }

  @Override
  public Grouping chips() {
    return chips;
  }

  @Override
  public Grouping systems() {
    return systems;
  }

  @Override
  @Nullable
  public Pack findPack(String id, Visitor<Track> visitor) throws IOException {
    return findPackInternal("/packs/pack/" + id, visitor);
  }

  @Override
  @Nullable
  public Pack findRandomPack(Visitor<Track> visitor) throws IOException {
    return findPackInternal("/packs/random", visitor);
  }

  @Nullable
  private Pack findPackInternal(String path, Visitor<Track> visitor) throws IOException {
    final Document doc = readDoc(buildUri(path));
    final String id = getSuffix(doc.selectFirst("meta[property=og:url]").attr("content"),
        "/packs/pack/");
    final Element titleEl = doc.selectFirst("div.row>section>h1");
    final Pack result = makePack(id, titleEl != null ? titleEl.text() : "");
    if (result == null) {
      return null;
    }
    parsePackRating(doc, result);
    final Elements tracks = doc.select("table.playlist>tbody>tr:has(>td.title)");
    for (Element track : tracks) {
      final int number = tryParseIntPrefix(track.attr("data-order"));
      final String title = track.selectFirst("td.title>a.beginPlay").text();
      final TimeStamp duration = TimeStamp.fromSeconds(tryParseIntPrefix(track.attr("data-duration")));
      final String location = Uri.decode(getSuffix(track.attr("data-vgmurl"), "/packs/vgm/"));
      if (number > 0 && !title.isEmpty() && !location.isEmpty()) {
        visitor.accept(new Track(number, title, duration, location));
        ++result.songs;
      }
    }
    return result;
  }

  final boolean isAvailable() {
    return http.hasConnection();
  }

  public static Uri[] getRemoteUris(Track track) {
    return new Uri[]{
        Cdn.vgmrips(track.location),
        buildBaseUri("packs/vgm/" + track.location).build()
    };
  }

  private static Uri buildUri(String path) {
    return buildBaseUri(path).build();
  }

  private static Uri buildPagedUri(String path, int page) {
    return buildBaseUri(path).appendQueryParameter("p", Integer.toString(page)).build();
  }

  private static Uri.Builder buildBaseUri(String path) {
    return new Uri.Builder()
        .scheme("https")
        .authority("vgmrips.net")
        .path(path);
  }

  private Document readDoc(Uri uri) throws IOException {
    return HtmlUtils.parseDoc(http.getInputStream(uri));
  }

  private class CompaniesGrouping implements Catalog.Grouping {

    private static final String DIR_PATH = "/packs/companies/";
    private static final String FILE_PREFIX = "/packs/company/";

    @Override
    public void query(Visitor<Group> visitor) throws IOException {
      final Document doc = readDoc(buildUri(DIR_PATH));
      final Elements list = doc.select("table>tbody>tr>td.link>a[href*=net/packs/company]");
      for (Element ref : list) {
        final String id = extractId(ref, FILE_PREFIX);
        final String title = ref.attr("title");
        final int packs = getPacksFromBadgeOrZero(ref.parent().parent(), "a[href$=/developed]");
        if (!id.isEmpty() && !title.isEmpty() && packs != 0) {
          visitor.accept(new Group(id, title, packs));
        }
      }
    }

    @Override
    public void queryPacks(String id, Visitor<Pack> visitor, ProgressCallback progress) throws IOException {
      final String path = FILE_PREFIX + id + "/developed";
      parsePacks(path, visitor, progress);
    }
  }

  private class ComposersGrouping implements Catalog.Grouping {

    private static final String DIR_PATH = "/packs/composers/";
    private static final String FILE_PREFIX = "/packs/composer/";

    @Override
    public void query(Visitor<Group> visitor) throws IOException {
      final Document doc = readDoc(buildUri(DIR_PATH));
      final Elements list = doc.select("li.composer>a[href*=net/packs/composer/]");
      for (Element ref : list) {
        final String id = extractId(ref, FILE_PREFIX);
        final String name = ref.ownText();
        final int packs = getPacksFromBadgeOrZero(ref);
        if (!id.isEmpty() && !name.isEmpty() && packs != 0) {
          visitor.accept(new Group(id, name, packs));
        }
      }
    }

    @Override
    public void queryPacks(String id, Visitor<Pack> visitor, ProgressCallback progress) throws IOException {
      final String path = FILE_PREFIX + id;
      parsePacks(path, visitor, progress);
    }
  }

  private class ChipsGrouping implements Catalog.Grouping {

    private static final String DIR_PATH = "/packs/chips/";
    private static final String FILE_PREFIX = "/packs/chip/";

    @Override
    public void query(Visitor<Group> visitor) throws IOException {
      final Document doc = readDoc(buildUri(DIR_PATH));
      final Elements list = doc.select("div.chip>a[href*=net/packs/chip/]");
      for (Element ref : list) {
        final String id = extractId(ref, FILE_PREFIX);
        final String name = ref.ownText();
        final int packs = getPacksFromBadgeOrZero(ref.parent());
        if (!id.isEmpty() && !name.isEmpty() && packs != 0) {
          visitor.accept(new Group(id, name, packs));
        }
      }
    }

    @Override
    public void queryPacks(String id, Visitor<Pack> visitor, ProgressCallback progress) throws IOException {
      final String path = FILE_PREFIX + id;
      parsePacks(path, visitor, progress);
    }
  }

  private class SystemsGrouping implements Catalog.Grouping {

    private static final String DIR_PATH = "/packs/systems/";
    private static final String FILE_PREFIX = "/packs/system/";
    private static final String TITLE_PREFIX = "View games on ";

    @Override
    public void query(Visitor<Group> visitor) throws IOException {
      final Document doc = readDoc(buildUri(DIR_PATH));
      final Elements list = doc.select("a[href*=net/packs/system/]:has(>img,div>span)");
      for (Element el : list) {
        final String id = extractId(el, FILE_PREFIX);
        final String title = getSuffix(el.attr("title"), TITLE_PREFIX);
        final int packs = getPacksFromBadgeOrZero(el);
        if (!id.isEmpty() && !title.isEmpty() && packs != 0) {
          visitor.accept(new Group(id, title, packs));
        }
      }
    }

    @Override
    public void queryPacks(String id, Visitor<Pack> visitor, ProgressCallback progress) throws IOException {
      final String path = FILE_PREFIX + id;
      parsePacks(path, visitor, progress);
    }
  }

  private String extractId(Element ref, String prefix) {
    final String url = ref.attr("href");
    return getSuffix(url, prefix);
  }

  private static String getSuffix(String str, String prefix) {
    final int prefixPos = str.indexOf(prefix);
    return prefixPos != -1
        ? str.substring(prefixPos + prefix.length())
        : "";
  }

  private static int getPacksFromBadgeOrZero(Element el) {
    return getPacksFromBadgeOrZero(el, "span.badge");
  }

  private static int getPacksFromBadgeOrZero(Element el, String query) {
    final Element badge = el.selectFirst(query);
    if (badge == null) {
      return 0;
    }
    return tryParseIntPrefix(badge.text());
  }

  private static int tryParseIntPrefix(String txt) {
    int idx = 0;
    for (int len = txt.length(); idx < len; ++idx) {
      if (!Character.isDigit(txt.charAt(idx))) {
        break;
      }
    }
    return idx != 0 ? Integer.parseInt(txt.substring(0, idx)) : 0;
  }

  private void parsePacks(String path, Visitor<Pack> visitor, ProgressCallback progress) throws IOException {
    for (int page = 0; ; ++page) {
      final Pair<Integer, Integer> result = parsePacksPage(buildPagedUri(path, page), visitor);
      if (result == null) {
        break;
      } else if (result.first != null && result.second != null) {
        progress.onProgressUpdate(result.first, result.second);
      }
    }
  }

  // done, total
  @Nullable
  private Pair<Integer, Integer> parsePacksPage(Uri uri, Visitor<Pack> visitor) throws IOException {
    final Document doc = readDoc(uri);
    final Pair<Integer, Integer> result = parsePacksProgress(doc);
    if (result == null) {
      return result;
    }
    for (Element el : doc.select("div.result:has(>div.image,div.details)")) {
      final Pack pack = parsePack(el);
      if (pack != null) {
        visitor.accept(pack);
      }
    }
    return result;
  }

  @Nullable
  private Pair<Integer, Integer> parsePacksProgress(Document doc) {
    final Element container = doc.selectFirst("div.container:has(>div.clearfix)");
    if (container == null) {
      return null;
    }
    for (TextNode node : container.textNodes()) {
      final String text = node.text();
      if (!text.startsWith("Packs")) {
        continue;
      }
      // [Packs, XX, to, YY, of, ZZ, total]
      final String[] elems = TextUtils.split(text, " ");
      if (elems.length == 7) {
        final Integer done = HtmlUtils.tryGetInteger(elems[3]);
        final Integer total = HtmlUtils.tryGetInteger(elems[5]);
        return new Pair<>(done, total);
      }
    }
    return null;
  }

  @Nullable
  private static Pack parsePack(Element el) {
    final Pack result = parsePackDetails(el.selectFirst("div.details"));
    if (result != null) {
      parsePackRating(el.selectFirst("div.image:has(>a:has(>img))"), result);
    }
    return result;
  }

  @Nullable
  private static Pack parsePackDetails(@Nullable Element el) {
    if (el == null) {
      return null;
    }
    final Pack pack = parseIdAndTitle(el.selectFirst("a[href*=/packs/pack/]:gt(0)"));
    if (pack != null) {
      parsePackPathSizeAndTracks(el.selectFirst("a.download[href*=/files/]"), pack);
    }
    return pack;
  }

  @Nullable
  private static Pack parseIdAndTitle(@Nullable Element el) {
    if (el != null) {
      final String id = getSuffix(el.attr("href"), "/packs/pack/");
      final String title = el.text();
      return makePack(id, title);
    } else {
      return null;
    }
  }

  @Nullable
  private static Pack makePack(String id, String title) {
    if (!id.isEmpty() && !title.isEmpty()) {
      return new Pack(id, title);
    } else {
      return null;
    }
  }

  private static void parsePackPathSizeAndTracks(@Nullable Element el, Pack result) {
    if (el != null) {
      final String[] elements = TextUtils.split(el.text(), " ");
      //[X,songs,*.Y,KB]
      if (elements.length == 5) {
        result.songs = tryParseIntPrefix(elements[0]);
      }
    }
  }

  private static void parsePackRating(@Nullable Element el, Pack result) {
    if (el == null) {
      return;
    }
    final Element rating = el.selectFirst("div.stars");
    result.score = parsePackScore(rating.classNames());
    result.ratings = tryParseIntPrefix(rating.nextElementSibling().text());
  }

  private static int parsePackScore(Iterable<String> classNames) {
    final String PREFIX = "stars";
    for (String cl : classNames) {
      if (!cl.equals(PREFIX) && cl.startsWith(PREFIX)) {
        return tryParseIntPrefix(cl.substring(PREFIX.length()));
      }
    }
    return -1;
  }
}
