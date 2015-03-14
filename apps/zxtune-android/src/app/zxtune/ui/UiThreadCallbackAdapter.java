/**
 *
 * @file
 *
 * @brief Callback proxy redirecting calls in UI thread
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.Activity;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;

public final class UiThreadCallbackAdapter implements Callback {
  
  private final Activity activity;
  private final Callback delegate;
  
  public UiThreadCallbackAdapter(Activity activity, Callback delegate) {
    this.activity = activity;
    this.delegate = delegate;
  }

  @Override
  public void onStatusChanged(final boolean isPlaying) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onStatusChanged(isPlaying);
      }
    });
  }

  @Override
  public void onItemChanged(final Item item) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onItemChanged(item);
      }
    });
  }
  
  @Override
  public void onIOStatusChanged(final boolean isActive) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onIOStatusChanged(isActive);
      }
    });
  }
  
  @Override
  public void onError(final String error) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onError(error);
      }
    });
  }
}
