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
  PlaybackControl control;

  public PlaybackControlsView(View view) {
    this.prev = (ImageButton) view.findViewById(R.id.controls_prev);
    this.playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    this.next = (ImageButton) view.findViewById(R.id.controls_next);
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
  }
    
  public void setControls(PlaybackControl ctrl) {
    control = ctrl != null ? ctrl : PlaybackControlStub.instance();
    //updateStatus();TODO
  }
  
  public void updateStatus() {
    final boolean playing = control.isPlaying();
    playPause.setImageResource(playing ? R.drawable.ic_pause : R.drawable.ic_play);
  }
}
