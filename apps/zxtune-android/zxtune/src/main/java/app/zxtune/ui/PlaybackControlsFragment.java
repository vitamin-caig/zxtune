/**
 * @file
 * @brief Playback controls logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.os.Bundle;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import app.zxtune.R;
import app.zxtune.device.media.MediaModel;
import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.ui.utils.UiUtils;

public class PlaybackControlsFragment extends Fragment {

  @Nullable
  private ImageButton playPause;
  @Nullable
  private ImageButton sequenceMode;

  @Nullable
  private MediaControllerCompat.TransportControls ctrl;
  private boolean isPlaying = false;
  private int sequenceModeValue = 0;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    final MediaModel model = MediaModel.of(getActivity());
    model.getController().observe(this, controller -> {
      if (controller != null) {
        ctrl = controller.getTransportControls();
        sequenceModeValue = controller.getShuffleMode();
        updateSequenceModeStatus();
      } else {
        ctrl = null;
      }
      UiUtils.setViewEnabled(getView(), controller != null);
    });
    model.getPlaybackState().observe(this, state -> {
      final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
      updateStatus(isPlaying);
    });
  }

  @Override
  @Nullable
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.controls, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    view.findViewById(R.id.controls_prev).setOnClickListener(v -> prev());
    playPause = view.findViewById(R.id.controls_play_pause);
    playPause.setOnClickListener(v -> playPause());
    view.findViewById(R.id.controls_next).setOnClickListener(v -> next());
    sequenceMode = view.findViewById(R.id.controls_sequence_mode);
    sequenceMode.setOnClickListener(v -> toggleSequenceMode());
  }

  private void prev() {
    ctrl.skipToPrevious();
  }

  private void playPause() {
    if (isPlaying) {
      ctrl.stop();
    } else {
      ctrl.play();
    }
  }

  private void next() {
    ctrl.skipToNext();
  }

  private void toggleSequenceMode() {
    sequenceModeValue = (sequenceModeValue + 1) % SequenceMode.values().length;
    ctrl.setShuffleMode(sequenceModeValue);
    updateSequenceModeStatus();
  }

  private void updateStatus(boolean isPlaying) {
    final int icon = isPlaying ? R.drawable.ic_pause : R.drawable.ic_play;
    playPause.setImageResource(icon);
    this.isPlaying = isPlaying;
  }

  private void updateSequenceModeStatus() {
    if (sequenceModeValue == SequenceMode.LOOPED.ordinal()) {
      sequenceMode.setImageResource(R.drawable.ic_sequence_looped);
    } else if (sequenceModeValue == SequenceMode.SHUFFLE.ordinal()) {
      sequenceMode.setImageResource(R.drawable.ic_sequence_shuffle);
    } else {
      sequenceMode.setImageResource(R.drawable.ic_sequence_ordered);
    }
  }
}
