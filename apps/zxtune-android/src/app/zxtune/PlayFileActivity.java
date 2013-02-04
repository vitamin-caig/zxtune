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

public class PlayFileActivity extends Activity {

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    final String fileToPlay = Uri.parse(getIntent().toUri(0)).getPath();
    startService(new Intent(Intent.ACTION_VIEW, Uri.parse(fileToPlay), this, PlaybackService.class));
    startActivity(new Intent(this, CurrentlyPlayingActivity.class));
    finish();
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
  }
}
