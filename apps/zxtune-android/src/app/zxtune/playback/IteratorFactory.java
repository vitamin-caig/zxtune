/**
 *
 * @file
 *
 * @brief Iterators factory
 *
 * @author vitamin.caig@gmail.com
 *
 */
package app.zxtune.playback;

import java.io.IOException;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

public final class IteratorFactory {

  /**
   * 
   * @param context Operational context
   * @param uri Object identifier- playlist entry/file/playlist file/folder
   * @return new iterator
   * @throws IOException
   */
  public static Iterator createIterator(Context context, Uri uri) throws IOException {
    if (uri.getScheme().equals(ContentResolver.SCHEME_CONTENT)) {
      return new PlaylistIterator(context, uri);
    } else {
      Iterator result = PlaylistFileIterator.create(context, uri);
      if (result == null) {
        final Uri[] uris = {uri};
        result = new FileIterator(context, uris);
      }
      return result;
    }
  }
  
  /**
   * 
   * @param context Operational context
   * @param uris List of objects identifiers. In case of multiple, used as files/folders identifiers 
   * @return new iterator
   * @throws IOException
   */
  public static Iterator createIterator(Context context, Uri[] uris) throws IOException {
    if (uris.length == 1) {
      return createIterator(context, uris[0]);
    } else {
      return new FileIterator(context, uris);
    }
  }
  
  private IteratorFactory() {
  }
}
