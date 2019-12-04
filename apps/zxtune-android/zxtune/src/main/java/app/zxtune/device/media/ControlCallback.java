package app.zxtune.device.media;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.media.session.MediaSessionCompat;

import app.zxtune.BroadcastReceiverConnection;
import app.zxtune.Log;
import app.zxtune.MainService;
import app.zxtune.Releaseable;
import app.zxtune.ReleaseableStub;
import app.zxtune.ScanService;
import app.zxtune.TimeStamp;
import app.zxtune.device.sound.SoundOutputSamplesTarget;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.playback.stubs.PlayableItemStub;
import app.zxtune.playback.SeekControl;

import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

//! Handle focus only for explicit start/stop calls
// TODO: handle implicit start/stop calls
class ControlCallback extends MediaSessionCompat.Callback {

  private static final String TAG = ControlCallback.class.getName();

  private final Context ctx;
  private final AudioManager manager;
  private final AudioManager.OnAudioFocusChangeListener focusListener;
  private final AtomicReference<Releaseable> noisyConnection;
  private final PlaybackServiceLocal svc;
  private final PlaybackControl ctrl;
  private final SeekControl seek;
  private final MediaSessionCompat session;

  ControlCallback(Context ctx, PlaybackServiceLocal svc, MediaSessionCompat session) {
    this.ctx = ctx;
    this.manager = (AudioManager) ctx.getSystemService(Context.AUDIO_SERVICE);
    this.focusListener = new AudioFocusChangeListener();
    this.noisyConnection = new AtomicReference<>(ReleaseableStub.instance());
    this.svc = svc;
    this.ctrl = svc.getPlaybackControl();
    this.seek = svc.getSeekControl();
    this.session = session;
  }

  @Override
  public void onPlay() {
    if (connectToAudioSystem()) {
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
    disconnectFromAudioSystem();
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
  public void onSeekTo(long ms) {
    try {
      seek.setPosition(TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS));
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to seek");
    }
  }

  @Override
  public void onCustomAction(String action, Bundle extra) {
    if (MainService.CUSTOM_ACTION_ADD_CURRENT.equals(action)) {
      addCurrent();
    } else if (MainService.CUSTOM_ACTION_ADD.equals(action)) {
      add(extra.getParcelableArray("uris"));
    }
  }

  private void addCurrent() {
    final Item current = svc.getNowPlaying();
    if (current != PlayableItemStub.instance()) {
      ScanService.add(ctx, current);
    }
  }

  private void add(Parcelable[] params) {
    final Uri[] uris = new Uri[params.length];
    for (int idx = 0, lim = params.length; idx < lim; ++idx) {
      uris[idx] = (Uri) params[idx];
    }
    ScanService.add(ctx, uris);
  }

  @Override
  public void onSetShuffleMode(int mode) {
    ctrl.setSequenceMode(PlaybackControl.SequenceMode.values()[mode]);
    session.setShuffleMode(mode);
  }

  @Override
  public void onSetRepeatMode(int mode) {
    ctrl.setTrackMode(PlaybackControl.TrackMode.values()[mode]);
    session.setRepeatMode(mode);
  }

  @Override
  public void onPlayFromUri(Uri uri, Bundle extras) {
    svc.setNowPlaying(uri);
  }

  private boolean connectToAudioSystem() {
    if (gainFocus()) {
      registerNoisyReceiver();
      return true;
    }
    return false;
  }

  private void disconnectFromAudioSystem() {
    unregisterNoisyReceiver();
    releaseFocus();
  }

  private boolean gainFocus() {
    return AudioManager.AUDIOFOCUS_REQUEST_GRANTED == manager.requestAudioFocus(focusListener,
        SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN);
  }

  private void releaseFocus() {
    manager.abandonAudioFocus(focusListener);
  }

  private class AudioFocusChangeListener implements AudioManager.OnAudioFocusChangeListener {
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
  }

  private void onFocusLost() {
    ctrl.stop();
  }

  private void onFocusRestore() {
    ctrl.play();
  }

  private void registerNoisyReceiver() {
    final BroadcastReceiver receiver = new BroadcastReceiver() {
      @Override
      public void onReceive(Context context, Intent intent) {
        if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {
          ctrl.stop();
        }
      }
    };
    final IntentFilter filter =
        new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
    noisyConnection.getAndSet(new BroadcastReceiverConnection(ctx, receiver, filter)).release();
  }

  private void unregisterNoisyReceiver() {
    noisyConnection.getAndSet(ReleaseableStub.instance()).release();
  }
}
