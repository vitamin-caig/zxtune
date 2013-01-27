/*
 * @file
 * 
 * @brief Activity to play single file
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.SeekBar;
import android.util.Log;

public class PlayFileActivity extends Activity {

  static final private String TAG = "zxtune";

  private String fileToPlay;
  private BroadcastReceiver receiver;
  private SeekBar position;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.play_file);

    fileToPlay = Uri.parse(getIntent().toUri(0)).getPath();
    Log.d(TAG, "Passed intent with file " + fileToPlay);

    position = (SeekBar) findViewById(R.id.play_position);

    receiver = new EventReceiver();
    final IntentFilter filter = new IntentFilter(Service.POSITION_UPDATE);

    registerReceiver(receiver, filter);
    
    if (fileToPlay.length() != 0) {
      play();
    }
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    unregisterReceiver(receiver);
  }

  public void onClick(View v) {
    switch (v.getId()) {
      case R.id.play_button:
        play();
        break;
      case R.id.stop_button:
        stop();
        break;
    }
  }

  private void play() {
    if (fileToPlay.length() == 0) {
      new AlertDialog.Builder(this).setTitle(
          "Run it via file browser and select on of the supported file types").show();
    } else {
      stop();
      final Intent intent = new Intent(this, Service.class);
      intent.putExtra(Service.FILE_NAME, fileToPlay);
      startService(intent);
    }
  }

  private void stop() {
    stopService(new Intent(this, Service.class));
  }

  class EventReceiver extends BroadcastReceiver {

    public void onReceive(Context context, Intent intent) {
      final int curFrame = intent.getIntExtra(Service.CURRENT_FRAME, 0);
      final int allFrames = intent.getIntExtra(Service.TOTAL_FRAMES, 0);
      position.setMax(allFrames);
      position.setProgress(curFrame);
    }
  }
}
