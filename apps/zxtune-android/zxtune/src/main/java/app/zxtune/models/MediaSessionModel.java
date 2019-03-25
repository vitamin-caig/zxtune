package app.zxtune.models;

import android.app.Application;
import android.arch.lifecycle.AndroidViewModel;
import android.arch.lifecycle.LiveData;
import android.arch.lifecycle.MutableLiveData;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.playback.Visualizer;
import app.zxtune.rpc.ParcelableBinder;
import app.zxtune.rpc.VisualizerProxy;

public final class MediaSessionModel extends AndroidViewModel {

  private final MutableLiveData<PlaybackStateCompat> playbackState;
  private final MutableLiveData<MediaMetadataCompat> mediaMetadata;
  private final MutableLiveData<MediaControllerCompat> mediaController;
  private final MutableLiveData<Visualizer> visualizer;

  public MediaSessionModel(@NonNull Application app) {
    super(app);
    this.playbackState = new MutableLiveData<>();
    this.mediaMetadata = new MutableLiveData<>();
    this.mediaController = new MutableLiveData<>();
    this.visualizer = new MutableLiveData<>();

    setControl(null);
  }

  final void setControl(@Nullable MediaControllerCompat ctrl) {
    this.mediaController.setValue(ctrl);
    if (ctrl != null) {
      playbackState.setValue(ctrl.getPlaybackState());
      mediaMetadata.setValue(ctrl.getMetadata());
      visualizer.setValue(extractVisualizer(ctrl.getExtras()));
      ctrl.registerCallback(new ControllerCallback());
    } else {
      playbackState.setValue(null);
      mediaMetadata.setValue(null);
      visualizer.setValue(null);
    }
  }

  private static Visualizer extractVisualizer(Bundle extras) {
    //required for proper deserialization
    extras.setClassLoader(ParcelableBinder.class.getClassLoader());
    final IBinder binder = ParcelableBinder.deserialize(extras.getParcelable(Visualizer.class.getName()));
    return VisualizerProxy.getClient(binder);
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
