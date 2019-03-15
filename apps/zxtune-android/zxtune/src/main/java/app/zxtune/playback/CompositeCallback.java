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
  private PlaybackControl.State lastState;
  private Item lastItem;

  public CompositeCallback() {
    this.delegates = new LinkedList<>();
    this.lastState = PlaybackControl.State.STOPPED;
  }

  @Override
  public void onInitialState(PlaybackControl.State state, Item item) {
    lastState = state;
    lastItem = item;
  }

  @Override
  public void onStateChanged(PlaybackControl.State state) {
    synchronized (delegates) {
      lastState = state;
      for (Callback cb : delegates) {
        cb.onStateChanged(state);
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

  @Override
  public void onError(String e) {
    synchronized (delegates) {
      for (Callback cb : delegates) {
        cb.onError(e);
      }
    }
  }

  public boolean isEmpty() {
    synchronized(delegates) {
      return delegates.isEmpty();
    }
  }

  public void add(Callback callback) {
    synchronized (delegates) {
      delegates.add(callback);
      callback.onInitialState(lastState, lastItem);
    }
  }

  public void remove(Callback callback) {
    synchronized (delegates) {
      delegates.remove(callback);
    }
  }
}
