/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.joshw;

import android.text.Html;
import android.text.TextUtils;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.fs.HttpProvider;

class RemoteCatalog extends Catalog {

  //private static final String TAG = RemoteCatalog.class.getName();
  private static final String STORAGE_FORMAT = "http://%s.joshw.info/%s";
  private static final byte[] HTML_SIGNATURE = {'<', '!', 'D', 'O', 'C', 'T', 'Y', 'P', 'E'};
  private static final String DIR_MARKUP = "DIR";
  private static final String FILE_MARKUP = "   ";
  // $1 - DIR or empty
  // $2 - name
  // $3 - size
  private static final Pattern ENTRIES =
          Pattern.compile("alt=\"\\[(" + DIR_MARKUP + "|" + FILE_MARKUP + ")\\]\"> " + //type
                          "<a href=\"([^/\"]+).+?</a>.+?" + //name
                          "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}.+?" + //date+time
                          "([\\d.]+[KM]?|-)",//size
                  Pattern.DOTALL);

  private final HttpProvider http;

  public RemoteCatalog(HttpProvider http) {
    this.http = http;
  }

  @Override
  public ByteBuffer getFileContent(List<String> path) throws IOException {
    final String catalogue = path.get(0);
    final List<String> localPath = path.subList(1, path.size());
    final String remotePath = String.format(Locale.US, STORAGE_FORMAT, catalogue, TextUtils.join("/", localPath));
    return http.getContent(remotePath);
  }

  @Override
  public boolean isDirContent(ByteBuffer buf) {
    final byte[] head = new byte[HTML_SIGNATURE.length];
    final int readBytes = buf.get(head).position();
    buf.position(0);
    return readBytes == head.length && Arrays.equals(head, HTML_SIGNATURE);
  }

  @Override
  public void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException {
    if (!isDirContent(data)) {
      throw new IOException("Not a directory file");
    }
    final String chars = toString(data);
    final Matcher matcher = ENTRIES.matcher(chars);
    while (matcher.find()) {
      final String dirMark = matcher.group(1);
      final String name = decodeHtml(matcher.group(2));
      if (DIR_MARKUP.equals(dirMark)) {
        visitor.acceptDir(name);
      } else {
        final String size = matcher.group(3);
        visitor.acceptFile(name, size);
      }
    }
  }

  private static String toString(ByteBuffer data) throws IOException {
    if (data.hasArray()) {
      return new String(data.array(), "UTF-8");
    } else {
      final byte[] buff = new byte[data.remaining()];
      data.get(buff);
      data.position(0);
      return new String(buff, "UTF-8");
    }
  }

  private static String decodeHtml(String txt) {
    return Html.fromHtml(txt).toString();
  }
}
