package app.zxtune.playback.service;

import android.content.Context;
import android.media.AudioManager;

import app.zxtune.Log;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.stubs.CallbackStub;
import app.zxtune.sound.SoundOutputSamplesTarget;

class AudioFocusHandler extends CallbackStub implements AudioManager.OnAudioFocusChangeListener {

  private static final String TAG = AudioFocusHandler.class.getName();

  private static enum State {
    STOPPED,
    PLAYING,
    PAUSED,
    FOCUS_LOST,
  }

  private final AudioManager manager;
  private final PlaybackControl control;
  private State state;

  AudioFocusHandler(Context ctx, PlaybackControl ctrl) {
    this.manager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    this.control = ctrl;
    this.state = State.STOPPED;
  }

  @Override
  public void onStateChanged(PlaybackControl.State state) {
    if (state == PlaybackControl.State.PLAYING) {
      onPlay();
    } else if (state == PlaybackControl.State.PAUSED) {
      onPause();
    } else if (state == PlaybackControl.State.STOPPED) {
      onStop();
    }
  }

  @Override
  public void onAudioFocusChange(int focusChange) {
    if (state == State.PAUSED) {
      return;
    }
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

  private void onPlay() {
    if (state == State.STOPPED) {
      gainFocus();
    }
    state = State.PLAYING;
  }

  private void onPause() {
    if (state == State.PLAYING) {
      state = state.PAUSED;
    }
  }

  private void onStop() {
    Log.d(TAG, "Release audio focus");
    releaseFocus();
    state = State.STOPPED;
  }


  private void gainFocus() {
    if (AudioManager.AUDIOFOCUS_REQUEST_GRANTED == manager.requestAudioFocus(this,
        SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN)) {
      Log.d(TAG, "Gained audio focus");
    } else {
      Log.d(TAG, "Failed to gain audio focus");
    }
  }

  private void releaseFocus() {
    manager.abandonAudioFocus(this);
  }

  private void onFocusLost() {
    state = State.FOCUS_LOST;
    control.pause();
  }

  private void onFocusRestore() {
    control.play();
  }
}
