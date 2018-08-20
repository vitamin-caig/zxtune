package app.zxtune.fs.httpdir;

import android.net.Uri;
import android.support.annotation.NonNull;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.MultisourceHttpProvider;
import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Locale;

final class RemoteCatalog extends Catalog {

  private final MultisourceHttpProvider http;

  RemoteCatalog(HttpProvider http) {
    this.http = new MultisourceHttpProvider(http);
  }

  @NonNull
  @Override
  public ByteBuffer getFileContent(Path path) throws IOException {
    return http.getContent(path.getRemoteUris());
  }

  final void getFileContent(Path path, OutputStream target) throws IOException {
    http.getContent(path.getRemoteUris(), target);
  }

  @Override
  public void parseDir(Path path, DirVisitor visitor) throws IOException {
    final ByteBuffer data = getFileContent(path);
    parseDir(data, visitor);
  }

  private void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException {
    final Document doc = Jsoup.parse(new ByteArrayInputStream(data.array()), null, "");
    if (parseXmlIndex(doc, visitor) || parseTableMarkup(doc, visitor) || parsePreMarkup(doc, visitor)) {
      return;
    }
    throw new IOException("Unsupported format of html page");
  }

  /*
  <list>
    <file mtime="YYYY-MM-DDThh:mm:ssZ" size="bytes">name</file>
    <directory mtime="YYYY-MM-DDThh:mm:ssZ">name</directory>
  </list>
   */
  private boolean parseXmlIndex(Document doc, DirVisitor visitor) {
    final Element list = doc.selectFirst("list");
    if (list == null) {
      return false;
    }
    for (Element row : list.select("directory")) {
      final String name = row.text();
      visitor.acceptDir(name);
    }
    for (Element row : list.select("file")) {
      final String name = row.text();
      final String sizeInBytes = row.attr("size");
      visitor.acceptFile(name, formatSize(Integer.parseInt(sizeInBytes)));
    }
    return true;
  }

  private static String formatSize(int v) {
    if (v < 1024) {
      return Integer.toString(v);
    } else {
      final int z = (31 - Integer.numberOfLeadingZeros(v)) / 10;
      return String.format(Locale.US, "%.1f%s", (float) v / (1 << (z * 10)), " KMG".charAt(z));
    }
  }

  /*
      <table>
         ...
         <tr>
           ...
           <td>
             ...
             <a href="%-encoded name">name</a>
           </td>
           ...
           <td>
             ...
               " size "
             ...
           </td>
           ...
         </tr>
         ...
       </table>
   */
  private boolean parseTableMarkup(Document doc, DirVisitor visitor) {
    boolean result = false;
    for (Element row : doc.select("table tr:not(th)")) {
      final Element name = row.selectFirst("td a");
      if (name == null) {
        continue;
      }
      result = true;
      final String nameVal = findEntryHref(name);
      if (nameVal == null) {
        continue;
      }
      final Element size = row.selectFirst("td:has(a) ~ td:matches(^ *([0-9.]+[KM]?) *$)");
      if (size != null) {
        visitor.acceptFile(nameVal, size.text().trim());
      } else {
        visitor.acceptDir(trimTrailingSlash(nameVal));
      }
    }
    return result;
  }

  /*
      <pre>
        ...
        <img ...><a href="%-encoded">name</a>... "date time size"
        ...
      </pre>
  */
  private boolean parsePreMarkup(Document doc, DirVisitor visitor) {
    final String DIR_SIZE = "-";
    boolean result = false;
    for (Element name : doc.select("pre > img + a")) {
      result = true;
      final String nameVal = findEntryHref(name);
      if (nameVal == null) {
        continue;
      }
      final String dateTimeSize = name.nextSibling().toString().trim();
      final int lastSpace = dateTimeSize.lastIndexOf(" ");
      final String sizeVal = dateTimeSize.substring(lastSpace + 1);
      if (DIR_SIZE.equals(sizeVal)) {
        visitor.acceptDir(trimTrailingSlash(nameVal));
      } else {
        visitor.acceptFile(nameVal, sizeVal);
      }
    }
    return result;
  }

  private static String findEntryHref(Element anchor) {
    final String href = Uri.decode(anchor.attr("href"));
    final String text = anchor.text();
    //text may be truncated/ellipsized/etc
    final int MIN_MATCHED_SYMBOLS = 16;
    return href.regionMatches(0, text, 0, Math.min(href.length(), MIN_MATCHED_SYMBOLS))
        ? href
        : null;
  }

  private static String trimTrailingSlash(String val) {
    return val.endsWith("/")
        ? val.substring(0, val.length() - 1)
        : val;
  }
}
