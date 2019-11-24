package app.zxtune.models;

import android.app.Application;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.playback.PlaylistControl;
import app.zxtune.playback.Visualizer;
import app.zxtune.rpc.ParcelableBinder;
import app.zxtune.rpc.PlaylistProxy;
import app.zxtune.rpc.VisualizerProxy;

public final class MediaSessionModel extends AndroidViewModel {

  private final MutableLiveData<PlaybackStateCompat> playbackState;
  private final MutableLiveData<MediaMetadataCompat> mediaMetadata;
  private final MutableLiveData<MediaControllerCompat> mediaController;
  private final MutableLiveData<PlaylistControl> playlist;
  private final MutableLiveData<Visualizer> visualizer;

  public MediaSessionModel(@NonNull Application app) {
    super(app);
    this.playbackState = new MutableLiveData<>();
    this.mediaMetadata = new MutableLiveData<>();
    this.mediaController = new MutableLiveData<>();
    this.playlist = new MutableLiveData<>();
    this.visualizer = new MutableLiveData<>();

    setControl(null);
  }

  final void setControl(@Nullable MediaControllerCompat ctrl) {
    this.mediaController.setValue(ctrl);
    if (ctrl != null) {
      playbackState.setValue(ctrl.getPlaybackState());
      mediaMetadata.setValue(ctrl.getMetadata());
      extractInterfaces(ctrl.getExtras());
      ctrl.registerCallback(new ControllerCallback());
    } else {
      playbackState.setValue(null);
      mediaMetadata.setValue(null);
      playlist.setValue(null);
      visualizer.setValue(null);
    }
  }

  private void extractInterfaces(Bundle extras) {
    if (Build.VERSION.SDK_INT >= 18) {
      setPlaylist(extras.getBinder(PlaylistControl.class.getName()));
      setVisualizer(extras.getBinder(Visualizer.class.getName()));
    } else {
      //required for proper deserialization
      extras.setClassLoader(ParcelableBinder.class.getClassLoader());
      setPlaylist(ParcelableBinder.deserialize(extras.getParcelable(PlaylistControl.class.getName())));
      setVisualizer(ParcelableBinder.deserialize(extras.getParcelable(Visualizer.class.getName())));
    }
  }

  private void setPlaylist(IBinder playlist) {
    this.playlist.setValue(PlaylistProxy.getClient(playlist));
  }

  private void setVisualizer(IBinder visualizer) {
    this.visualizer.setValue(VisualizerProxy.getClient(visualizer));
  }

  @NonNull
  public final LiveData<PlaybackStateCompat> getState() {
    return playbackState;
  }

  @NonNull
  public final LiveData<MediaMetadataCompat> getMetadata() {
    return mediaMetadata;
  }

  @NonNull
  public final LiveData<MediaControllerCompat> getMediaController() {
    return mediaController;
  }

  @NonNull
  public final LiveData<PlaylistControl> getPlaylist() {
    return playlist;
  }

  @NonNull
  public final LiveData<Visualizer> getVisualizer() {
    return visualizer;
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
