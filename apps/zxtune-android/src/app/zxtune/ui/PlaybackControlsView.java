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
import app.zxtune.R;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackControl.TrackMode;
import app.zxtune.playback.PlaybackControlStub;

class PlaybackControlsView {
  
  private final ImageButton prev;
  private final ImageButton playPause;
  private final ImageButton next;
  private final ImageButton loop;
  private PlaybackControl control;

  PlaybackControlsView(View view) {
    this.prev = (ImageButton) view.findViewById(R.id.controls_prev);
    this.playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    this.next = (ImageButton) view.findViewById(R.id.controls_next);
    this.loop = (ImageButton) view.findViewById(R.id.controls_loop);
    
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
    
    loop.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        final TrackMode[] availModes = TrackMode.values();
        final TrackMode curMode = control.getTrackMode();
        TrackMode newMode = curMode == availModes[availModes.length - 1]
          ? availModes[0]
          : availModes[curMode.ordinal() + 1];
        control.setTrackMode(newMode);
        updateLoopStatus();
      }
    });
  }
    
  final void setControls(PlaybackControl ctrl) {
    control = ctrl != null ? ctrl : PlaybackControlStub.instance();
    //updateStatus();TODO
  }
  
  final void updateStatus(boolean playing) {
    final int level = playing ? 1 : 0;
    playPause.getDrawable().setLevel(level);
    updateLoopStatus();
  }
  
  private void updateLoopStatus() {
    final int level = control.getTrackMode().ordinal();
    loop.getDrawable().setLevel(level);
  }
}
