package app.zxtune.playback.service;

import android.support.annotation.NonNull;

import app.zxtune.Log;
import app.zxtune.playback.Callback;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.sound.PlayerEventsListener;

class PlaybackEvents implements PlayerEventsListener {

  private static final String TAG = PlaybackEvents.class.getName();

  private final Callback callback;
  private final PlaybackControl ctrl;

  PlaybackEvents(Callback callback, PlaybackControl ctrl) {
    this.callback = callback;
    this.ctrl = ctrl;
  }

  @Override
  public void onStart() {
    callback.onStateChanged(PlaybackControl.State.PLAYING);
  }

  @Override
  public void onFinish() {
    try {
      ctrl.next();
    } catch (Exception e) {
      onError(e);
    }
  }

  @Override
  public void onStop() {
    callback.onStateChanged(PlaybackControl.State.STOPPED);
  }

  @Override
  public void onError(@NonNull Exception e) {
    Log.w(TAG, e, "Error occurred");
  }
}
