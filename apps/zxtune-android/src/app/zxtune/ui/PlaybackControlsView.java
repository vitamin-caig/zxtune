/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import app.zxtune.R;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackControlStub;

public class PlaybackControlsView {
  
  final ImageButton prev;
  final ImageButton playPause;
  final ImageButton next;
  final ImageButton loop;
  PlaybackControl control;

  public PlaybackControlsView(View view) {
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
        control.setLooped(!control.isLooped());
        updateLoopStatus();
      }
    });
  }
    
  public void setControls(PlaybackControl ctrl) {
    control = ctrl != null ? ctrl : PlaybackControlStub.instance();
    //updateStatus();TODO
  }
  
  public void updateStatus(boolean playing) {
    playPause.setImageResource(playing ? R.drawable.ic_pause : R.drawable.ic_play);
    updateLoopStatus();
  }
  
  private void updateLoopStatus() {
    loop.setImageResource(control.isLooped() ? R.drawable.ic_loop : R.drawable.ic_sequence);
  }
}
