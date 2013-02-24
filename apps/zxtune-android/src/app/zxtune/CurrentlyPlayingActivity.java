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
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.view.View;

public class CurrentlyPlayingActivity extends FragmentActivity {

  private Playback.Control control;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.currently_playing);

    control = MessengerRPC.ControlClient.create(this);
    
    if (savedInstanceState == null) {
      createView();
    }
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    MessengerRPC.ControlClient.destroy(control);
  }

  private void createView() {
    final Fragment seek = new app.zxtune.ui.Position(control);
    final Fragment ctrl = new app.zxtune.ui.Controls(control);
    getSupportFragmentManager().beginTransaction()
        .replace(R.id.position_view, seek)
        .replace(R.id.controls_view, ctrl)
        .commit();
  }
}
