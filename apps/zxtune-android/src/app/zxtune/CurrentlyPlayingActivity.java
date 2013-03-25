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
import android.support.v4.app.FragmentManager;
import android.view.View;
import app.zxtune.fs.Provider;
import app.zxtune.fs.Vfs;
import android.util.Log;
import app.zxtune.ui.*;

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
    getPart(Position.class).setControl(control);
    getPart(Controls.class).setControl(control);
    getPart(Playlist.class).setControl(control);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    MessengerRPC.ControlClient.destroy(control);
  }

  private final void createView() {
    final Fragment seek = new Position();
    final Fragment ctrl = new Controls();
    final Fragment browser = new Browser();
    final Fragment playlist = new Playlist();
    getSupportFragmentManager().beginTransaction()
        .replace(R.id.position_view, seek, seek.getClass().getName())
        .replace(R.id.controls_view, ctrl, ctrl.getClass().getName())
        .replace(R.id.browser_view, browser, browser.getClass().getName())
        .replace(R.id.playlist_view, playlist, playlist.getClass().getName())
        .commit();
  }
  
  private final <T extends Fragment> T getPart(Class<T> type) {
    final FragmentManager mgr = getSupportFragmentManager(); 
    mgr.executePendingTransactions();
    return (T) mgr.findFragmentByTag(type.getName());
  }
}
