/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.aygor;

import android.net.Uri;
import android.text.TextUtils;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import app.zxtune.fs.HttpProvider;

class RemoteCatalog extends Catalog {

  //private static final String TAG = RemoteCatalog.class.getName();
  //use tagged names for html content
  static final String VERSION = "ayon";
  private static final String STORAGE_MIRROR = "http://abrimaal.pro-e.pl/ayon/";
  private static final byte[] HTML_SIGNATURE = {'<', '!', 'D', 'O', 'C', 'T', 'Y', 'P', 'E'};
  private static final String DIR_MARKUP = "DIR";
  private static final String FILE_MARKUP = "   ";
  // $1 - DIR or empty
  // $2 - name
  // $3 - size
  private static final Pattern ENTRIES =
          Pattern.compile("alt=\"\\[(" + DIR_MARKUP + "|" + FILE_MARKUP + ")\\]\"></td><td>" + //type
                          "<a href=\"([^/\"]+).+?</a>.+?" + //name
                          "\\d{2}-[A-Za-z]{3}-\\d{4} \\d{2}:\\d{2}.+?" + //date
                          "([\\d.]+[KM]?|-)",//size
                  Pattern.DOTALL);

  private final HttpProvider http;

  public RemoteCatalog(HttpProvider http) {
    this.http = http;
  }

  @Override
  public ByteBuffer getFileContent(List<String> path) throws IOException {
    final String pathStr = TextUtils.join("/", path);
    return http.getContent(STORAGE_MIRROR + pathStr);
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
      throw new UnsupportedOperationException();
    }
    final String chars = toString(data);
    final Matcher matcher = ENTRIES.matcher(chars);
    while (matcher.find()) {
      final String dirMark = matcher.group(1);
      final String name = Uri.decode(matcher.group(2));
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
}
