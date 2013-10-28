/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import android.net.Uri;

public class PlaylistControlStub implements PlaylistControl {

  private PlaylistControlStub() {
    // TODO Auto-generated constructor stub
  }

  @Override
  public void add(Uri[] uris) {
  }
  
  @Override
  public void delete(long[] ids) {
  }

  @Override
  public void deleteAll() {
  }
  
  public static PlaylistControl instance() {
    return Holder.INSTANCE;
  }
  
  private static class Holder {
    public static final PlaylistControl INSTANCE = new PlaylistControlStub();
  }
}
