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
    callback.onStateChanged(PlaybackControl.State.PLAYING, getPos());
  }

  @Override
  public void onSeeking() {
    callback.onStateChanged(PlaybackControl.State.SEEKING, getPos());
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
    callback.onStateChanged(PlaybackControl.State.STOPPED, getPos());
  }

  @Override
  public void onError(Exception e) {
    Log.w(TAG, e, "Error occurred");
  }

  private TimeStamp getPos() {
    try {
      return seek.getPosition();
    } catch (Exception e) {
      return TimeStamp.EMPTY;
    }
  }
}
