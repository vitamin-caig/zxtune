package app.zxtune.fs.httpdir;

import android.net.Uri;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Locale;

import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.io.Io;

public class RemoteCatalog extends Catalog {

  private final MultisourceHttpProvider http;

  public RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
  }

  private ByteBuffer getFileContent(Path path) throws IOException {
    return Io.readFrom(http.getInputStream(path.getRemoteUris()));
  }

  @Override
  public void parseDir(Path path, DirVisitor visitor) throws IOException {
    final ByteBuffer data = getFileContent(path);
    parseDir(data, visitor);
  }

  // TODO: operate with streams
  void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException {
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
      final String dateTime = row.attr("mtime");
      visitor.acceptDir(name, dateTime);
    }
    for (Element row : list.select("file")) {
      final String name = row.text();
      final String dateTime = row.attr("mtime");
      final String sizeInBytes = row.attr("size");
      visitor.acceptFile(name, dateTime, formatSize(Long.parseLong(sizeInBytes)));
    }
    return true;
  }

  private static String formatSize(long v) {
    if (v < 1024) {
      return Long.toString(v);
    } else {
      final int z = (63 - Long.numberOfLeadingZeros(v)) / 10;
      return String.format(Locale.US, "%.1f%s", (float) v / (1 << (z * 10)), " KMGTPE".charAt(z));
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
      final Element datetime = row.selectFirst("td:has(a) ~ td:matches(^ *([0-9]{4}-[01][0-9]-[0-3][0-9] " +
                                                   "[0-5][0-9]:[0-5][0-9]) *$)");
      final String description = datetime != null ? datetime.text().trim() : "";
      final Element size = row.selectFirst("td:has(a) ~ td:matches(^ *([0-9.]+[KM]?) *$)");
      if (size != null) {
        visitor.acceptFile(nameVal, description, size.text().trim());
      } else {
        visitor.acceptDir(trimTrailingSlash(nameVal), description);
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
      final String dateTimeVal = dateTimeSize.substring(0, lastSpace).trim();
      final String sizeVal = dateTimeSize.substring(lastSpace + 1).trim();
      if (DIR_SIZE.equals(sizeVal)) {
        visitor.acceptDir(trimTrailingSlash(nameVal), dateTimeVal);
      } else {
        visitor.acceptFile(nameVal, dateTimeVal, sizeVal);
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
