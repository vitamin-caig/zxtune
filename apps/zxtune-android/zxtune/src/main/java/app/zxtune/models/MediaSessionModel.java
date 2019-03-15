package app.zxtune.models;

import android.app.Application;
import android.arch.lifecycle.AndroidViewModel;
import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.MutableLiveData;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;

public final class MediaSessionModel extends AndroidViewModel {

  private final MutableLiveData<PlaybackStateCompat> playbackState;
  private final MutableLiveData<MediaMetadataCompat> mediaMetadata;
  private MediaControllerCompat ctrl;

  public MediaSessionModel(@NonNull Application app) {
    super(app);
    this.playbackState = new MutableLiveData<>();
    this.mediaMetadata = new MutableLiveData<>();

    setControl(null);
  }

  public final void setControl(@Nullable MediaControllerCompat ctrl) {
    this.ctrl = ctrl;
    if (ctrl != null) {
      playbackState.setValue(ctrl.getPlaybackState());
      mediaMetadata.setValue(ctrl.getMetadata());
      ctrl.registerCallback(new ControllerCallback());
    } else {
      playbackState.setValue(null);
      mediaMetadata.setValue(null);
    }
  }

  @NonNull
  public final LiveData<PlaybackStateCompat> getState() {
    return playbackState;
  }

  @NonNull
  public final LiveData<MediaMetadataCompat> getMetadata() {
    return mediaMetadata;
  }

  @Nullable
  public final MediaControllerCompat.TransportControls getTransportControls() {
    return ctrl != null ? ctrl.getTransportControls() : null;
  }

  private class ControllerCallback extends MediaControllerCompat.Callback {
    @Override
    public void onPlaybackStateChanged(PlaybackStateCompat state) {
      playbackState.setValue(state);
    }

    @Override
    public void onMetadataChanged(MediaMetadataCompat metadata) {
      mediaMetadata.setValue(metadata);
    }
  }
}
