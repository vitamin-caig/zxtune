/**
 * @file
 * @brief Base composite callback implementation
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */
package app.zxtune.playback;

import java.util.LinkedList;
import java.util.List;

import android.util.Log;
import app.zxtune.Releaseable;

public final class CompositeCallback implements Callback {
  
  private static final String TAG = CompositeCallback.class.getName();
  private final List<Callback> delegates;
  private Control lastControl;
  
  public CompositeCallback() {
    this.delegates = new LinkedList<Callback>();
  }
  
  @Override
  public void onControlChanged(Control control) {
    synchronized (delegates) {
      for (Callback cb : delegates) {
        cb.onControlChanged(control);
      }
      lastControl = control;
    }
  }

  @Override
  public void onStatusChanged(boolean isPlaying) {
    synchronized (delegates) {
      for (Callback cb : delegates) {
        cb.onStatusChanged(isPlaying);
      }
    }
  }

  @Override
  public void onItemChanged(Item item) {
    synchronized (delegates) {
      for (Callback cb : delegates) {
        cb.onItemChanged(item);
      }
    }
  }

  public CompositeCallback add(Callback callback) {
    synchronized (delegates) {
      delegates.add(callback);
      if (lastControl != null) {
        callback.onControlChanged(lastControl);
      }
    }
    Log.d(TAG, "Added " + callback.toString());
    return this;
  }

  private void remove(Callback callback) {
    synchronized (delegates) {
      delegates.remove(callback);
      callback.onControlChanged(null);
    }
    Log.d(TAG, "Removed " + callback.toString());
  }

  public Releaseable subscribe(Callback delegate) {
    return new Connection(delegate);
  }

  private class Connection implements Releaseable {

    private Callback delegate;

    public Connection(Callback delegate) {
      this.delegate = delegate;
      add(delegate);
    }

    @Override
    public void release() {
      remove(delegate);
      delegate = null;
    }
  }
}
