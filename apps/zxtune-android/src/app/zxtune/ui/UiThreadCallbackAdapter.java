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
  public void onStatusChanged(boolean isPlaying) {
    activity.runOnUiThread(new UpdateStatusRunnable(isPlaying));
  }

  @Override
  public void onItemChanged(Item item) {
    activity.runOnUiThread(new UpdateItemRunnable(item));
  }
  
  @Override
  public void onIOStatusChanged(boolean isActive) {
    activity.runOnUiThread(new UpdateIOStatusRunnable(isActive));
  }
  
  private class UpdateStatusRunnable implements Runnable {
    
    private final boolean isPlaying;
    
    UpdateStatusRunnable(boolean isPlaying) {
      this.isPlaying = isPlaying;
    }
    
    @Override
    public void run() {
      delegate.onStatusChanged(isPlaying);
    }
  }

  private class UpdateItemRunnable implements Runnable {
    
    private final Item item;
    
    UpdateItemRunnable(Item item) {
      this.item = item;
    }
    
    @Override
    public void run() {
      delegate.onItemChanged(item);
    }
  }

  private class UpdateIOStatusRunnable implements Runnable {
    
    private final boolean isActive;
    
    UpdateIOStatusRunnable(boolean isActive) {
      this.isActive = isActive;
    }
    
    @Override
    public void run() {
      delegate.onIOStatusChanged(isActive);
    }
  }
}
