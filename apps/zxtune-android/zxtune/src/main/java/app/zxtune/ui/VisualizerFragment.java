package app.zxtune.ui;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.R;
import app.zxtune.models.MediaSessionModel;
import app.zxtune.playback.Visualizer;
import app.zxtune.ui.views.SpectrumAnalyzerView;

public class VisualizerFragment extends Fragment {

  private SpectrumAnalyzerView view;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    final MediaSessionModel model = ViewModelProviders.of(getActivity()).get(MediaSessionModel.class);
    model.getVisualizer().observe(this, new Observer<Visualizer>() {
      @Override
      public void onChanged(@Nullable Visualizer visualizer) {
        view.setSource(visualizer);
      }
    });
    model.getState().observe(this, new Observer<PlaybackStateCompat>() {
      @Override
      public void onChanged(@Nullable PlaybackStateCompat state) {
        final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
        view.setIsUpdating(isPlaying);
      }
    });
  }

  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.visualizer, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    this.view = view.findViewById(R.id.spectrum);
  }

  @Override
  public void setUserVisibleHint(boolean isVisible) {
    super.setUserVisibleHint(isVisible);
  }

  @Override
  public void onHiddenChanged(boolean isHidden) {
    super.onHiddenChanged(isHidden);
  }
}
