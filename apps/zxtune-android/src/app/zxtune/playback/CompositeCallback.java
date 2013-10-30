/**
 *
 * @file
 *
 * @brief Composite callback helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import java.util.LinkedList;
import java.util.List;

public final class CompositeCallback implements Callback {
  
  private final List<Callback> delegates;
  private boolean lastStatus;
  private Item lastItem;
  
  public CompositeCallback() {
    this.delegates = new LinkedList<Callback>();
  }
  
  @Override
  public void onStatusChanged(boolean isPlaying) {
    synchronized (delegates) {
      lastStatus = isPlaying;
      for (Callback cb : delegates) {
        cb.onStatusChanged(isPlaying);
      }
    }
  }

  @Override
  public void onItemChanged(Item item) {
    synchronized (delegates) {
      lastItem = item;
      for (Callback cb : delegates) {
        cb.onItemChanged(item);
      }
    }
  }

  public int add(Callback callback) {
    synchronized (delegates) {
      delegates.add(callback);
      if (lastItem != null) {
        callback.onItemChanged(lastItem);
      }
      callback.onStatusChanged(lastStatus);
      return delegates.size();
    }
  }

  public int remove(Callback callback) {
    synchronized (delegates) {
      delegates.remove(callback);
      return delegates.size();
    }
  }
}
