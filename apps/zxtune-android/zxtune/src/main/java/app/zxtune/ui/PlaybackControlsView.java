/**
 *
 * @file
 *
 * @brief Playback controls logic
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ImageView;

import app.zxtune.R;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.playback.PlaybackControl.TrackMode;
import app.zxtune.playback.stubs.PlaybackControlStub;

class PlaybackControlsView {

  private final ImageButton playPause;
  private final ImageView trackMode;
  private final ImageButton sequenceMode;
  private PlaybackControl control;

  PlaybackControlsView(View view) {
    this.playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    this.trackMode = (ImageView) view.findViewById(R.id.controls_track_mode);
    this.sequenceMode = (ImageButton) view.findViewById(R.id.controls_sequence_mode);
    
    control = PlaybackControlStub.instance();

    final ImageButton prev = (ImageButton) view.findViewById(R.id.controls_prev);
    prev.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.prev();
      }
    });
    playPause.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        if (PlaybackControl.State.PLAYING == control.getState()) {
          control.stop();
        } else {
          control.play();
        }
      }
    });
    final ImageButton next = (ImageButton) view.findViewById(R.id.controls_next);
    next.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.next();
      }
    });
    
    trackMode.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        final TrackMode[] availModes = TrackMode.values();
        final TrackMode curMode = control.getTrackMode();
        final TrackMode newMode = curMode == availModes[availModes.length - 1]
          ? availModes[0]
          : availModes[curMode.ordinal() + 1];
        control.setTrackMode(newMode);
        updateTrackModeStatus();
      }
    });
    sequenceMode.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        final SequenceMode[] availModes = SequenceMode.values();
        final SequenceMode curMode = control.getSequenceMode();
        final SequenceMode newMode = curMode == availModes[availModes.length - 1]
                ? availModes[0]
                : availModes[curMode.ordinal() + 1];
        control.setSequenceMode(newMode);
        updateSequenceModeStatus();
      }
    });
  }
    
  final void setControls(PlaybackControl ctrl) {
    control = ctrl != null ? ctrl : PlaybackControlStub.instance();
    //updateStatus();TODO
    updateSequenceModeStatus();
  }
  
  final void updateStatus(boolean playing) {
    playPause.setImageResource(playing ? R.drawable.ic_pause : R.drawable.ic_play);
    updateTrackModeStatus();
  }
  
  private void updateTrackModeStatus() {
    final boolean looped = control.getTrackMode() == TrackMode.LOOPED;
    trackMode.setImageResource(looped ? R.drawable.ic_track_looped : R.drawable.ic_track_regular);
  }
  
  private void updateSequenceModeStatus() {
    switch (control.getSequenceMode()) {
      case LOOPED:
        sequenceMode.setImageResource(R.drawable.ic_sequence_looped);
        break;
      case SHUFFLE:
        sequenceMode.setImageResource(R.drawable.ic_sequence_shuffle);
        break;
      default:
        sequenceMode.setImageResource(R.drawable.ic_sequence_ordered);
        break;
    }
  }
}
