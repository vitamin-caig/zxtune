package app.zxtune.models;

import android.app.Application;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;

import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProvider;

import app.zxtune.playback.Visualizer;
import app.zxtune.rpc.ParcelableBinder;
import app.zxtune.rpc.VisualizerProxy;

// public for provider
public final class MediaSessionModel extends AndroidViewModel {

  private final MutableLiveData<PlaybackStateCompat> playbackState;
  private final MutableLiveData<MediaMetadataCompat> mediaMetadata;
  private final MutableLiveData<MediaControllerCompat> mediaController;
  private final MutableLiveData<Visualizer> visualizer;

  public static MediaSessionModel of(FragmentActivity owner) {
    return new ViewModelProvider(owner,
        ViewModelProvider.AndroidViewModelFactory.getInstance(owner.getApplication())).get(MediaSessionModel.class);
  }

  public MediaSessionModel(Application app) {
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
      extractInterfaces(ctrl.getExtras());
      ctrl.registerCallback(new ControllerCallback());
    } else {
      playbackState.setValue(null);
      mediaMetadata.setValue(null);
      visualizer.setValue(null);
    }
  }

  private void extractInterfaces(Bundle extras) {
    if (Build.VERSION.SDK_INT >= 18) {
      setVisualizer(extras.getBinder(Visualizer.class.getName()));
    } else {
      //required for proper deserialization
      extras.setClassLoader(ParcelableBinder.class.getClassLoader());
      setVisualizer(ParcelableBinder.deserialize(extras.getParcelable(Visualizer.class.getName())));
    }
  }

  private void setVisualizer(@Nullable IBinder visualizer) {
    if (visualizer != null) {
      this.visualizer.setValue(VisualizerProxy.getClient(visualizer));
    } else {
      this.visualizer.setValue(null);
    }
  }

  public final LiveData<PlaybackStateCompat> getState() {
    return playbackState;
  }

  public final LiveData<MediaMetadataCompat> getMetadata() {
    return mediaMetadata;
  }

  public final LiveData<MediaControllerCompat> getMediaController() {
    return mediaController;
  }

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
