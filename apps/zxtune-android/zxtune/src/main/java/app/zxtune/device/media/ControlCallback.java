package app.zxtune.device.media;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.media.session.MediaSessionCompat;

import androidx.annotation.Nullable;

import app.zxtune.Log;
import app.zxtune.MainService;
import app.zxtune.ScanService;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.playback.stubs.PlayableItemStub;

class ControlCallback extends MediaSessionCompat.Callback {

  private static final String TAG = ControlCallback.class.getName();

  private final Context ctx;
  private final PlaybackServiceLocal svc;
  private final PlaybackControl ctrl;
  private final SeekControl seek;
  private final MediaSessionCompat session;

  ControlCallback(Context ctx, PlaybackServiceLocal svc, MediaSessionCompat session) {
    this.ctx = ctx;
    this.svc = svc;
    this.ctrl = svc.getPlaybackControl();
    this.seek = svc.getSeekControl();
    this.session = session;
  }

  @Override
  public void onPlay() {
    ctrl.play();
  }

  @Override
  public void onPause() {
    onStop();
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

  @Override
  public void onSeekTo(long ms) {
    try {
      seek.setPosition(TimeStamp.fromMilliseconds(ms));
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

  private void add(@Nullable Parcelable[] params) {
    if (params != null) {
      final Uri[] uris = new Uri[params.length];
      for (int idx = 0, lim = params.length; idx < lim; ++idx) {
        uris[idx] = (Uri) params[idx];
      }
      ScanService.add(ctx, uris);
    }
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
}
