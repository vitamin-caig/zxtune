/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlaylistControl
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

public class PlaylistControlStub implements PlaylistControl {

  private PlaylistControlStub() {
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
  
  @Override
  public void move(long id, int delta) {
  }
  
  public static PlaylistControl instance() {
    return Holder.INSTANCE;
  }
  
  private static class Holder {
    public static final PlaylistControl INSTANCE = new PlaylistControlStub();
  }
}
