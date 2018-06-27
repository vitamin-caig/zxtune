package app.zxtune.fs.httpdir;

import android.net.Uri;
import android.support.annotation.NonNull;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;

import app.zxtune.fs.http.HttpProvider;

final class RemoteCatalog extends Catalog {

  private static final byte[] HTML_SIGNATURE = {'<', '!', 'D', 'O', 'C', 'T', 'Y', 'P', 'E'};

  private final HttpProvider http;

  RemoteCatalog(HttpProvider http) {
    this.http = http;
  }

  @NonNull
  @Override
  public ByteBuffer getFileContent(Path path) throws IOException {
    return http.getContent(path.getRemoteUri());
  }

  final void getFileContent(Path path, OutputStream target) throws IOException {
    http.getContent(path.getRemoteUri(), target);
  }

  @Override
  public void parseDir(Path path, DirVisitor visitor) throws IOException {
    final ByteBuffer data = getFileContent(path);
    if (!isDirContent(data)) {
      throw new IOException("Not a directory file");
    }
    parseDir(data, visitor);
  }

  private static boolean isDirContent(ByteBuffer buf) {
    final byte[] head = new byte[HTML_SIGNATURE.length];
    final int readBytes = buf.get(head).position();
    buf.position(0);
    return readBytes == head.length && Arrays.equals(head, HTML_SIGNATURE);
  }

  private void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException {
    final Document doc = Jsoup.parse(new ByteArrayInputStream(data.array()), null, "");
    if (parseTableMarkup(doc, visitor) || parsePreMarkup(doc, visitor)) {
      return;
    }
    throw new IOException("Unsupported format of html page");
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
  private boolean parseTableMarkup(Document doc, DirVisitor visitor) throws IOException {
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
  private boolean parsePreMarkup(Document doc, DirVisitor visitor) throws IOException {
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
