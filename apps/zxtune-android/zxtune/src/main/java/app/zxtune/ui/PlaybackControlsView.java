/**
 * @file
 * @brief Playback controls logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ImageView;
import app.zxtune.R;
import app.zxtune.models.MediaSessionModel;
import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.playback.PlaybackControl.TrackMode;

class PlaybackControlsView {

  private final FragmentActivity activity;
  private final ImageButton playPause;
  private final OnClickListener playOnClick;
  private final OnClickListener pauseOnClick;
  private final ImageView trackMode;
  private final ImageButton sequenceMode;

  PlaybackControlsView(FragmentActivity activity, View view) {
    this.activity = activity;
    this.playPause = view.findViewById(R.id.controls_play_pause);
    this.trackMode = view.findViewById(R.id.controls_track_mode);
    this.sequenceMode = view.findViewById(R.id.controls_sequence_mode);

    final ImageButton prev = view.findViewById(R.id.controls_prev);
    prev.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        prev();
      }
    });
    playOnClick = new OnClickListener() {
      @Override
      public void onClick(View v) {
        play();
      }
    };
    pauseOnClick = new OnClickListener() {
      @Override
      public void onClick(View v) {
        stop();
      }
    };
    final ImageButton next = view.findViewById(R.id.controls_next);
    next.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        next();
      }
    });

    trackMode.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        toggleTrackMode();
      }
    });
    sequenceMode.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        toggleSequenceMode();
      }
    });
    final MediaSessionModel model = ViewModelProviders.of(activity).get(MediaSessionModel.class);
    model.getState().observe(activity, new Observer<PlaybackStateCompat>() {
      @Override
      public void onChanged(@Nullable PlaybackStateCompat state) {
        final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
        updateStatus(isPlaying);
      }
    });
  }

  private void prev() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      ctrl.getTransportControls().skipToPrevious();
    }
  }

  private void play() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      ctrl.getTransportControls().play();
    }
  }

  private void stop() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      ctrl.getTransportControls().stop();
    }
  }

  private void next() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      ctrl.getTransportControls().skipToNext();
    }
  }

  private void toggleTrackMode() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      int mode = ctrl.getRepeatMode();
      mode = (mode + 1) % TrackMode.values().length;
      ctrl.getTransportControls().setRepeatMode(mode);
      updateTrackModeStatus(mode);
    }
  }

  private void toggleSequenceMode() {
    final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
    if (ctrl != null) {
      int mode = ctrl.getShuffleMode();
      mode = (mode + 1) % SequenceMode.values().length;
      ctrl.getTransportControls().setShuffleMode(mode);
      updateSequenceModeStatus(mode);
    }
  }

  private final void updateStatus(boolean isPlaying) {
    if (isPlaying) {
      playPause.setImageResource(R.drawable.ic_pause);
      playPause.setOnClickListener(pauseOnClick);
    } else {
      playPause.setImageResource(R.drawable.ic_play);
      playPause.setOnClickListener(playOnClick);
    }
  }

  private void updateTrackModeStatus(int mode) {
    final boolean looped = mode == TrackMode.LOOPED.ordinal();
    trackMode.setImageResource(looped ? R.drawable.ic_track_looped : R.drawable.ic_track_regular);
  }

  private void updateSequenceModeStatus(int mode) {
    if (mode == SequenceMode.LOOPED.ordinal()) {
      sequenceMode.setImageResource(R.drawable.ic_sequence_looped);
    } else if (mode == SequenceMode.SHUFFLE.ordinal()) {
      sequenceMode.setImageResource(R.drawable.ic_sequence_shuffle);
    } else {
      sequenceMode.setImageResource(R.drawable.ic_sequence_ordered);
    }
  }
}
