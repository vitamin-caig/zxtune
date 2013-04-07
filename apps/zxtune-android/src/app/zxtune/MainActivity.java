/*
 * @file
 * @brief Currently playing activity
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.IOException;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.rpc.PlaybackControlClient;
import app.zxtune.ui.BrowserFragment;
import app.zxtune.ui.NowPlayingFragment;
import app.zxtune.ui.PlaylistFragment;

public class MainActivity extends FragmentActivity {

  private final PlaybackControlClient.ConnectionHandler connectionHandler;
  private PlaybackControlClient control;

  public MainActivity() {
    this.connectionHandler = new ClientConnectionHandle();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main_activity);

    fillChildViews();
    final ViewPager pager = (ViewPager) findViewById(R.id.view_pager);
    final ViewGroup childs = (ViewGroup) findViewById(R.id.view_content);
    final PagerAdapter adapter = new Adapter(childs);
    pager.setAdapter(adapter);
    
    final Intent intent = new Intent(this, PlaybackService.class);
    PlaybackControlClient.create(this, intent, connectionHandler);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    try {
      if (control != null) {
        control.close();
      }
    } catch (IOException e) {} finally {
      control = null;
    }
  }

  private void fillChildViews() {
    final Fragment nowPlaying = new NowPlayingFragment();
    final Fragment browser = new BrowserFragment();
    final Fragment playlist = new PlaylistFragment();
    getSupportFragmentManager().beginTransaction()
        .replace(R.id.now_playing, nowPlaying, nowPlaying.getClass().getName())
        .replace(R.id.browser_view, browser, browser.getClass().getName())
        .replace(R.id.playlist_view, playlist, playlist.getClass().getName()).commit();
  }
  
  private class ClientConnectionHandle implements PlaybackControlClient.ConnectionHandler {

    @Override
    public void onConnected(PlaybackControlClient client) {
      control = client;
      getPart(NowPlayingFragment.class).setControl(control);
      getPart(PlaylistFragment.class).setControl(control);
      //TODO: get rid of
      ((ViewGroup) findViewById(R.id.view_content)).removeAllViews();
    }

    @Override
    public void onDisconnected() {
      onConnected(null);
    }
  }

  private static class Adapter extends PagerAdapter {

    private final View[] pages;

    public Adapter(ViewGroup childs) {
      this.pages = new View[childs.getChildCount()];
      for (int idx = 0; idx != this.pages.length; ++idx) {
        pages[idx] = childs.getChildAt(idx);
      }
    }

    @Override
    public int getCount() {
      return pages.length;
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
      return view == object;
    }

    @Override
    public Object instantiateItem(ViewGroup container, int position) {
      final View childView = pages[position];
      container.addView(childView);
      return childView;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
      container.removeView((View) object);
    }
  }
  
  private <T extends Fragment> T getPart(Class<T> type) {
    final FragmentManager mgr = getSupportFragmentManager();
    mgr.executePendingTransactions();
    return (T) mgr.findFragmentByTag(type.getName());
  }
}
