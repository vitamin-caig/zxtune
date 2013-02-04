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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.SeekBar;

public class CurrentlyPlayingActivity extends Activity {

  private MessengerRPC.Client service;
  private BroadcastReceiver receiver;
  private SeekBar position;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.currently_playing);

    service = MessengerRPC.Client.create(this); 
    position = (SeekBar) findViewById(R.id.play_position);
    receiver = new EventReceiver();
    final IntentFilter filter = new IntentFilter(PlaybackService.POSITION_UPDATE);

    registerReceiver(receiver, filter);
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
    service.disconnect();
    unregisterReceiver(receiver);
  }

  public void onClick(View v) {
    switch (v.getId()) {
      case R.id.play_button:
        service.play();
        break;
      case R.id.pause_button:
        service.pause();
        break;
      case R.id.stop_button:
        service.stop();
        break;
    }
  }

  class EventReceiver extends BroadcastReceiver {

    public void onReceive(Context context, Intent intent) {
      final int curFrame = intent.getIntExtra(PlaybackService.CURRENT_FRAME, 0);
      final int allFrames = intent.getIntExtra(PlaybackService.TOTAL_FRAMES, 0);
      position.setMax(allFrames);
      position.setProgress(curFrame);
    }
  }
}
