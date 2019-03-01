package app.zxtune.device.media;

import android.content.Context;
import android.media.AudioManager;
import android.support.v4.media.session.MediaSessionCompat;
import app.zxtune.Log;
import app.zxtune.device.sound.SoundOutputSamplesTarget;
import app.zxtune.playback.PlaybackControl;

//! Handle focus only for explicit start/stop calls
// TODO: handle implicit start/stop calls
class ControlCallback extends MediaSessionCompat.Callback implements AudioManager.OnAudioFocusChangeListener {

  private static final String TAG = ControlCallback.class.getName();

  private final AudioManager manager;
  private final PlaybackControl ctrl;

  ControlCallback(Context ctx, PlaybackControl ctrl) {
    this.manager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    this.ctrl = ctrl;
  }

  @Override
  public void onPlay() {
    if (gainFocus()) {
      ctrl.play();
    } else {
      Log.d(TAG, "Failed to gain focus");
    }
  }

  @Override
  public void onPause() {
    onStop();
  }

  @Override
  public void onStop() {
    releaseFocus();
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

  @Override
  public void onAudioFocusChange(int focusChange) {
    switch (focusChange) {
      case AudioManager.AUDIOFOCUS_LOSS:
        Log.d(TAG, "Focus lost");
        onFocusLost();
        break;
      case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
        Log.d(TAG, "Focus lost transient");
        onFocusLost();
        break;
      case AudioManager.AUDIOFOCUS_GAIN:
        Log.d(TAG, "Focus restored");
        onFocusRestore();
        break;
    }
  }

  private boolean gainFocus() {
    return AudioManager.AUDIOFOCUS_REQUEST_GRANTED == manager.requestAudioFocus(this,
        SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN);
  }

  private void releaseFocus() {
    manager.abandonAudioFocus(this);
  }

  private void onFocusLost() {
    ctrl.stop();
  }

  private void onFocusRestore() {
    ctrl.play();
  }
}
