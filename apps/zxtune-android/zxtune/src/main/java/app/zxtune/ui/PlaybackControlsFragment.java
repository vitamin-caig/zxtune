/**
 * @file
 * @brief Playback controls logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import app.zxtune.R;
import app.zxtune.models.MediaSessionModel;
import app.zxtune.playback.PlaybackControl.SequenceMode;

public class PlaybackControlsFragment extends Fragment {

  private ImageButton playPause;
  private ImageButton sequenceMode;

  private MediaControllerCompat.TransportControls ctrl;
  private boolean isPlaying = false;
  private int sequenceModeValue = 0;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    final MediaSessionModel model = ViewModelProviders.of(getActivity()).get(MediaSessionModel.class);
    model.getMediaController().observe(this, new Observer<MediaControllerCompat>() {
      @Override
      public void onChanged(@Nullable MediaControllerCompat controller) {
        if (controller != null) {
          ctrl = controller.getTransportControls();
          getView().setEnabled(true);
          sequenceModeValue = controller.getShuffleMode();
          updateSequenceModeStatus();
        } else {
          ctrl = null;
          getView().setEnabled(false);
        }
      }
    });
    model.getState().observe(this, new Observer<PlaybackStateCompat>() {
      @Override
      public void onChanged(@Nullable PlaybackStateCompat state) {
        final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
        updateStatus(isPlaying);
      }
    });
  }

  @Override
  @Nullable
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.controls, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    view.findViewById(R.id.controls_prev).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        prev();
      }
    });
    playPause = view.findViewById(R.id.controls_play_pause);
    playPause.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        playPause();
      }
    });
    view.findViewById(R.id.controls_next).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        next();
      }
    });
    sequenceMode = view.findViewById(R.id.controls_sequence_mode);
    sequenceMode.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        toggleSequenceMode();
      }
    });
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
