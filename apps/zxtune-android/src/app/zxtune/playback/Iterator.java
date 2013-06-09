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
   * @return Newly loaded item
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
      return new FileIterator(uri);
    }
  }
}
