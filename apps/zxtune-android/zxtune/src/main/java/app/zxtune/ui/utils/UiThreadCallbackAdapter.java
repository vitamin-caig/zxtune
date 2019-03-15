/**
 * @file
 * @brief Callback proxy redirecting calls in UI thread
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui.utils;

import android.support.v4.app.FragmentActivity;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;

public final class UiThreadCallbackAdapter implements Callback {

  private final FragmentActivity activity;
  private final Callback delegate;

  public UiThreadCallbackAdapter(FragmentActivity activity, Callback delegate) {
    this.activity = activity;
    this.delegate = delegate;
  }

  @Override
  public void onInitialState(final PlaybackControl.State state, final Item item) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onInitialState(state, item);
      }
    });
  }

  @Override
  public void onStateChanged(final PlaybackControl.State state) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onStateChanged(state);
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
  public void onError(final String error) {
    activity.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        delegate.onError(error);
      }
    });
  }
}
