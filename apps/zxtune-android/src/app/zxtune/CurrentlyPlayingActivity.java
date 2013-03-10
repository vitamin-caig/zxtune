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
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.view.View;
import app.zxtune.fs.Provider;
import app.zxtune.fs.Vfs;
import android.util.Log;
import app.zxtune.ui.*;

public class CurrentlyPlayingActivity extends FragmentActivity implements Browser.Callback {

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
    final Fragment seek = new Position(control);
    final Fragment ctrl = new Controls(control);
    final Fragment playlist = new Browser(this, this);
    getSupportFragmentManager().beginTransaction()
        .replace(R.id.position_view, seek)
        .replace(R.id.controls_view, ctrl)
        .replace(R.id.playlist_view, playlist)
        .commit();
  }
  
  @Override
  public void onFileSelected(Uri uri) {
    control.open(uri.getPath());
  }
}
