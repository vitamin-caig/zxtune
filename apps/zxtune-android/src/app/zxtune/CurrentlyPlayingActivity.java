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

import java.io.IOException;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import app.zxtune.rpc.PlaybackControlClient;
import app.zxtune.ui.Browser;
import app.zxtune.ui.Controls;
import app.zxtune.ui.Playlist;
import app.zxtune.ui.Position;

public class CurrentlyPlayingActivity extends FragmentActivity implements PlaybackControlClient.ConnectionHandler {

  private PlaybackControlClient control;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.currently_playing);

    if (savedInstanceState == null) {
      createView();
    }
    
    PlaybackControlClient.create(this, new Intent(this, PlaybackService.class), this);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    try {
      if (control != null) {
        control.close();
      }
    } catch (IOException e) {
    } finally {
      control = null;
    }
  }

  @Override
  public void onConnected(PlaybackControlClient client) {
    control = client;
    getPart(Controls.class).setControl(control);
    getPart(Playlist.class).setControl(control);
  }

  @Override
  public void onDisconnected() {
    onConnected(null);
  }
    
  private void createView() {
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
  
  private <T extends Fragment> T getPart(Class<T> type) {
    final FragmentManager mgr = getSupportFragmentManager(); 
    mgr.executePendingTransactions();
    return (T) mgr.findFragmentByTag(type.getName());
  }
}
