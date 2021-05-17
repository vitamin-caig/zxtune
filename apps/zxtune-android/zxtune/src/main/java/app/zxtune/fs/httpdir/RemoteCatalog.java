package app.zxtune.fs.httpdir;

import android.net.Uri;

import androidx.annotation.Nullable;

import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

import app.zxtune.Util;
import app.zxtune.fs.HtmlUtils;
import app.zxtune.fs.http.MultisourceHttpProvider;

public class RemoteCatalog implements Catalog {

  private final MultisourceHttpProvider http;

  public RemoteCatalog(MultisourceHttpProvider http) {
    this.http = http;
  }

  @Override
  public void parseDir(Path path, DirVisitor visitor) throws IOException {
    final InputStream input = http.getInputStream(path.getRemoteUris());
    parseDir(input, visitor);
  }

  // used in testing
  void parseDir(InputStream input, DirVisitor visitor) throws IOException {
    final Document doc = HtmlUtils.parseDoc(input);
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
      visitor.acceptFile(name, dateTime, Util.formatSize(Long.parseLong(sizeInBytes)));
    }
    return true;
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
      final Element ref = row.selectFirst("td a");
      if (ref == null) {
        continue;
      }
      result = true;
      final String nameVal = findEntryHref(ref);
      if (nameVal == null) {
        continue;
      }
      final String name = trimTrailingSlash(nameVal);
      if (name.equals("..") || name.equals(".")) {
        continue;
      }
      final Element datetime = row.selectFirst("td:has(a) ~ td:matches(^ *([0-9]{4}-" +
          "([01][0-9]|[JFMASOND][a-z][a-z])-[0-3][0-9] " +
          "[0-5][0-9](:[0-5][0-9])+) *$)");
      final String description = datetime != null ? datetime.text().trim() : "";
      final Element size = row.selectFirst("td:has(a) ~ td:matches(^ *([0-9.]+[KM]?) *$)");
      if (size != null) {
        visitor.acceptFile(name, description, size.text().trim());
      } else {
        visitor.acceptDir(name, description);
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

  @Nullable
  private static String findEntryHref(Element anchor) {
    final String href = Uri.decode(anchor.attr("href"));
    final String text = anchor.text();
    //text may be truncated/ellipsized/etc
    final int MIN_MATCHED_SYMBOLS = 16;
    return href.regionMatches(0, text, 0, Math.min(Math.min(href.length(), text.length()),
        MIN_MATCHED_SYMBOLS))
        ? href
        : null;
  }

  private static String trimTrailingSlash(String val) {
    return val.endsWith("/")
        ? val.substring(0, val.length() - 1)
        : val;
  }
}
