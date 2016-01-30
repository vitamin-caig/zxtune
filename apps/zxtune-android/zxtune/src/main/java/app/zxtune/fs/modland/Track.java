/**
 *
 * @file
 *
 * @brief Track description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import android.net.Uri;

import java.util.zip.CRC32;

public final class Track {

  /// unique id (hash of decoded path)
  public final long id;
  /// encoded path starting at /pub/modules/...
  /// encoding is not fully compatible with Uri.encode, so keep it as is
  public final String path;
  /// decoded last segment of id
  public final String filename;
  /// in bytes
  public final int size;

  public Track(String path, int size) {
    this(crc32(Uri.decode(path)), path, size);
  }

  public Track(long id, String path, int size) {
    this.id = id;
    this.path = path;
    this.filename = Uri.decode(path.substring(1 + path.lastIndexOf('/')));
    this.size = size;
  }

  private static long crc32(String str) {
    final CRC32 crc = new CRC32();
    crc.update(str.getBytes());
    return crc.getValue();
  }
}
