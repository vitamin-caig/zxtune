/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import java.util.concurrent.TimeUnit;

import android.os.Handler;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.SeekControlStub;

public class SeekControlView {
  
  private final TextView currentTime;
  private final TextView totalTime;
  private final SeekBar currentPosition;
  private final Handler timer;
  private final Runnable updateTask;
  private SeekControl control;

  public SeekControlView(View layout) {
    this.currentTime = (TextView) layout.findViewById(R.id.position_time);
    this.totalTime = (TextView) layout.findViewById(R.id.position_duration);
    this.currentPosition = (SeekBar) layout.findViewById(R.id.position_seek);
    this.timer = new Handler();
    this.updateTask = new UpdateViewTask();
    this.control = SeekControlStub.instance(); 
    this.currentPosition.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
          final TimeStamp pos = TimeStamp.createFrom(progress, TimeUnit.SECONDS);
          control.setPosition(pos);
        }
      }
      
      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {}
      
      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {}
    });
  }

  public void setControl(SeekControl control) {
    if (control == null) {
      timer.removeCallbacks(updateTask);
      this.control = SeekControlStub.instance();
      currentTime.setText(R.string.stub_time);
      totalTime.setText(R.string.stub_time);
      currentPosition.setProgress(0);
    } else {
      this.control = control;
      final TimeStamp duration = control.getDuration();
      totalTime.setText(duration.toString());
      currentPosition.setProgress(0);
      currentPosition.setMax((int) duration.convertTo(TimeUnit.SECONDS));
      updateTask.run();
    }
  }
  
  private final class UpdateViewTask implements Runnable {
    @Override
    public void run() {
      final TimeStamp pos = control.getPosition();
      currentPosition.setProgress((int) pos.convertTo(TimeUnit.SECONDS));
      currentTime.setText(pos.toString());
      
      timer.postDelayed(this, 1000);
    }
  }
}
