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

    final Uri fileToPlay = Uri.parse(getIntent().toUri(Intent.URI_INTENT_SCHEME));
    startService(new Intent(Intent.ACTION_VIEW, fileToPlay, this, PlaybackService.class));
    startActivity(new Intent(this, CurrentlyPlayingActivity.class));
    finish();
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
  }
}
