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
import app.zxtune.playback.PlaybackControl.TrackMode;
import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.playback.PlaybackControlStub;

class PlaybackControlsView {
  
  private final ImageButton prev;
  private final ImageButton playPause;
  private final ImageButton next;
  private final ImageView trackMode;
  private final ImageButton sequenceMode;
  private PlaybackControl control;

  PlaybackControlsView(View view) {
    this.prev = (ImageButton) view.findViewById(R.id.controls_prev);
    this.playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    this.next = (ImageButton) view.findViewById(R.id.controls_next);
    this.trackMode = (ImageView) view.findViewById(R.id.controls_track_mode);
    this.sequenceMode = (ImageButton) view.findViewById(R.id.controls_sequence_mode);
    
    control = PlaybackControlStub.instance();
    
    prev.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.prev();
      }
    });
    playPause.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        if (control.isPlaying()) {
          control.stop();
        } else {
          control.play();
        }
      }
    });
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
        TrackMode newMode = curMode == availModes[availModes.length - 1]
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
        SequenceMode newMode = curMode == availModes[availModes.length - 1]
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
    final int level = playing ? 1 : 0;
    playPause.getDrawable().setLevel(level);
    updateTrackModeStatus();
  }
  
  private void updateTrackModeStatus() {
    final int level = control.getTrackMode().ordinal();
    trackMode.getDrawable().setLevel(level);
  }
  
  private void updateSequenceModeStatus() {
    final int level = control.getSequenceMode().ordinal();
    sequenceMode.getDrawable().setLevel(level);
  }
}
