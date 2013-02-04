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
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;

public class PlayFileActivity extends Activity {

  private MessengerRPC.Client service;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    final String fileToPlay = Uri.parse(getIntent().toUri(0)).getPath();
    service = MessengerRPC.Client.create(this, fileToPlay);
    final Intent intent = new Intent(this, CurrentlyPlayingActivity.class);
    startActivity(intent);
    finish();
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
    service.disconnect();
  }
}
