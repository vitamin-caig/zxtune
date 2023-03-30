package app.zxtune.ui;

import android.os.Bundle;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import app.zxtune.R;
import app.zxtune.device.media.MediaModel;
import app.zxtune.ui.views.SpectrumAnalyzerView;

public class VisualizerFragment extends Fragment {

  @Nullable
  private SpectrumAnalyzerView view;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    final MediaModel model = MediaModel.of(getActivity());
    model.getVisualizer().observe(this, visualizer -> view.setSource(visualizer));
    model.getPlaybackState().observe(this, state -> {
      final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
      view.setIsUpdating(isPlaying);
    });
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.visualizer, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
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
