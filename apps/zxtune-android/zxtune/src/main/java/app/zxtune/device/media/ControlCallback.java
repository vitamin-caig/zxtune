package app.zxtune.device.media;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;
import app.zxtune.Log;
import app.zxtune.MainService;
import app.zxtune.ScanService;
import app.zxtune.device.sound.SoundOutputSamplesTarget;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.playback.stubs.PlayableItemStub;

//! Handle focus only for explicit start/stop calls
// TODO: handle implicit start/stop calls
class ControlCallback extends MediaSessionCompat.Callback implements AudioManager.OnAudioFocusChangeListener {

  private static final String TAG = ControlCallback.class.getName();

  private final Context ctx;
  private final AudioManager manager;
  private final PlaybackServiceLocal svc;
  private final PlaybackControl ctrl;

  ControlCallback(Context ctx, PlaybackServiceLocal svc) {
    this.ctx = ctx;
    this.manager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    this.svc = svc;
    this.ctrl = svc.getPlaybackControl();
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
  public void onCustomAction(String action, Bundle extra) {
    if (MainService.CUSTOM_ACTION_ADD_CURRENT.equals(action)) {
      addCurrent();
    }
  }

  private void addCurrent() {
    final Item current = svc.getNowPlaying();
    if (current != PlayableItemStub.instance()) {
      ScanService.add(ctx, current);
    }
  }

  //onAudioFocusChangeListener
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
