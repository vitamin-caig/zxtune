/**
 * @file
 * @brief Composite callback helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

import java.util.LinkedList;
import java.util.List;

import app.zxtune.Releaseable;
import app.zxtune.TimeStamp;

public final class CompositeCallback implements Callback {

  private final List<Callback> delegates;
  private PlaybackControl.State lastState;

  public CompositeCallback() {
    this.delegates = new LinkedList<>();
    this.lastState = PlaybackControl.State.STOPPED;
  }

  @Override
  public void onInitialState(PlaybackControl.State state) {
    lastState = state;
  }

  @Override
  public void onStateChanged(PlaybackControl.State state, TimeStamp pos) {
    synchronized(delegates) {
      lastState = state;
      for (Callback cb : delegates) {
        cb.onStateChanged(state, pos);
      }
    }
  }

  @Override
  public void onItemChanged(Item item) {
    synchronized(delegates) {
      for (Callback cb : delegates) {
        cb.onItemChanged(item);
      }
    }
  }

  @Override
  public void onError(String e) {
    synchronized(delegates) {
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

  public Releaseable add(Callback callback) {
    synchronized(delegates) {
      delegates.add(callback);
      callback.onInitialState(lastState);
    }
    return () -> {
      synchronized(delegates) {
        delegates.remove(callback);
      }
    };
  }
}
