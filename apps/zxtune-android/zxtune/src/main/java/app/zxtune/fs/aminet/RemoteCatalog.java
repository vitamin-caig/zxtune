package app.zxtune.fs.aminet;

import android.net.Uri;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.io.IOException;
import java.io.InputStream;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.httpdir.Path;

public class RemoteCatalog extends app.zxtune.fs.httpdir.RemoteCatalog {

  private final HttpProvider simpleHttp;

  public RemoteCatalog(HttpProvider http) {
    super(http);
    this.simpleHttp = http;
  }

  @Override
  public void parseDir(Path path, DirVisitor visitor) throws IOException {
    if (path.getParent() == null) {
      parseRoot(visitor);
    } else {
      parseDir(path.getLocalId(), visitor);
    }
  }

  private void parseRoot(DirVisitor visitor) throws IOException {
    final Document doc = Jsoup.parse(readRoot(), null, "");
    for (Element el : doc.select("li:has(a[href]:containsOwn(mods))")) {
      // mods/[author] - [desc]
      final String line = el.text();
      if (line.startsWith("mods/")) {
        final int separator = line.indexOf(" - ");
        final String name = line.substring(5, separator);
        final String desc = line.substring(separator + 3);
        visitor.acceptDir(name, desc);
      }
    }
  }

  private Uri.Builder getBase() {
    return new Uri.Builder()
               .scheme("http")
               .authority("aminet.net");
  }

  private InputStream readRoot() throws IOException {
    final Uri uri = getBase()
                        .path("tree")
                        .appendQueryParameter("path", "mods")
                        .build();
    return simpleHttp.getInputStream(uri);
  }

  private void parseDir(String path, DirVisitor visitor) throws IOException {
    final Uri.Builder base = getBase()
                                 .path("search")
                                 .appendQueryParameter("path", "mods/" + path);
    parseDir(base, visitor);
  }

  public final boolean searchSupported() {
    return simpleHttp.hasConnection();
  }

  public final void find(String query, DirVisitor visitor) throws IOException {
    final Uri.Builder base = getBase()
                                 .path("search")
                                 .appendQueryParameter("name", query)
                                 .appendQueryParameter("q_desc", "OR")
                                 .appendQueryParameter("desc", query)
                                 .appendQueryParameter("path[]", "mods");
    parseDir(base, visitor);
  }

  private void parseDir(Uri.Builder base, DirVisitor visitor) throws IOException {
    //http://aminet.net/search?name=disco&path[]=mods&q_desc=OR&desc=disco
    int start = 0;
    for (; ; ) {
      final Document doc = Jsoup.parse(readPage(base, start), null, "");
      final Elements listing = doc.select("tr.lightrow,tr.darkrow");
      if (listing.isEmpty()) {
        break;
      }
      for (Element el : listing) {
        final Elements tds = el.select("td>font");
        final Element nameEl = tds.get(0);
        final String path = nameEl.child(0).attr("href");
        if (path.startsWith("/mods/")) {
          final String size = tds.get(4).text();
          final String desc = tds.get(7).child(0).text();
          visitor.acceptFile(path.substring(5), desc, size);
        }
      }
      start += listing.size();
    }
  }

  private InputStream readPage(Uri.Builder base, int start) throws IOException {
    final Uri uri = base.appendQueryParameter("start", Integer.toString(start)).build();
    return simpleHttp.getInputStream(uri);
  }
}
