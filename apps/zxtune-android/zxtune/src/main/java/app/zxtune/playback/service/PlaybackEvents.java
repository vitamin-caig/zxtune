package app.zxtune.playback.service;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.SeekControl;
import app.zxtune.sound.PlayerEventsListener;

class PlaybackEvents implements PlayerEventsListener {

  private static final String TAG = PlaybackEvents.class.getName();

  private final Callback callback;
  private final PlaybackControl ctrl;
  private final SeekControl seek;

  PlaybackEvents(Callback callback, PlaybackControl ctrl, SeekControl seek) {
    this.callback = callback;
    this.ctrl = ctrl;
    this.seek = seek;
  }

  @Override
  public void onStart() {
    callback.onStateChanged(PlaybackControl.State.PLAYING);
  }

  @Override
  public void onFinish() {
    try {
      seek.setPosition(TimeStamp.EMPTY);
      ctrl.next();
    } catch (Exception e) {
      onError(e);
    }
  }

  @Override
  public void onPause() {
    callback.onStateChanged(PlaybackControl.State.PAUSED);
  }

  @Override
  public void onStop() {
    callback.onStateChanged(PlaybackControl.State.STOPPED);
  }

  @Override
  public void onError(Exception e) {
    Log.w(TAG, e, "Error occurred");
  }
}
