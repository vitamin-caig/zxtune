/*
 * @file
 * 
 * @brief Currently playing activity
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.SeekBar;

public class CurrentlyPlayingActivity extends Activity {

  private Playback.Control control;
  private final Playback.Callback callback = new StatusCallback();
  private SeekBar position;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.currently_playing);

    control = MessengerRPC.ControlClient.create(this);
    position = (SeekBar) findViewById(R.id.play_position);
  }

  @Override
  public void onStart() {
    super.onStart();
    control.registerCallback(callback);
  }

  @Override
  public void onStop() {
    super.onStop();
    control.unregisterCallback(callback);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    MessengerRPC.ControlClient.destroy(control);
  }

  public void onClick(View v) {
    switch (v.getId()) {
      case R.id.play_button:
        control.play();
        break;
      case R.id.pause_button:
        control.pause();
        break;
      case R.id.stop_button:
        control.stop();
        break;
    }
  }

  class StatusCallback implements Playback.Callback {
    public void started(String description, int duration) {
      position.setMax(duration);
    }

    public void paused(String description) {

    }

    public void stopped() {

    }

    public void positionChanged(int curFrame, String curTime) {
      position.setProgress(curFrame);
    }
  }
}
