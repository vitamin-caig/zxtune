/**
 * @file
 * @brief Playback iterator interface
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import java.io.IOException;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

public abstract class Iterator {

  /**
   *  Retrieve current position' item
   *  @return Newly loaded item
   */
  public abstract PlayableItem getItem();

  /**
   *  Move to next position
   *  @return true if successfully moved, else getItem will return the same item
   */
  public abstract boolean next();
  
  /**
   *  Move to previous position
   *  @return true if successfully moved, else getItem will return the same item
   */
  public abstract boolean prev();
  
  public static Iterator create(Context context, Uri uri) throws IOException {
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
  
  public static Iterator create(Context context, Uri[] uris) throws IOException {
    if (uris.length == 1) {
      return create(context, uris[0]);
    } else {
      return new FileIterator(context, uris);
    }
  }
}
