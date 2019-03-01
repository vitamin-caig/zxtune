package app.zxtune.device.media;

import android.support.v4.media.session.MediaSessionCompat;
import app.zxtune.playback.PlaybackControl;

class ControlCallback extends MediaSessionCompat.Callback {

  private final PlaybackControl ctrl;

  ControlCallback(PlaybackControl ctrl) {
    this.ctrl = ctrl;
  }

  @Override
  public void onPlay() {
    ctrl.play();
  }

  @Override
  public void onPause() {
    ctrl.stop();
  }

  @Override
  public void onStop() {
    ctrl.stop();
  }

  @Override
  public void onSkipToPrevious() {
    ctrl.prev();
  }

  @Override
  public void onSkipToNext() {
    ctrl.next();
  }
}
