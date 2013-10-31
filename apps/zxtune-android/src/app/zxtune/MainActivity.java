/**
 *
 * @file
 *
 * @brief Main application activity
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.playback.PlaybackService;
import app.zxtune.ui.AboutFragment;
import app.zxtune.ui.BrowserFragment;
import app.zxtune.ui.NowPlayingFragment;
import app.zxtune.ui.PlaylistFragment;

public class MainActivity extends ActionBarActivity implements PlaybackServiceConnection.Callback {
  
  private PlaybackService service;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main_activity);

    fillPages();
  }
  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    super.onCreateOptionsMenu(menu);
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }
  
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.action_prefs:
        showPreferences();
        break;
      case R.id.action_about:
        showAbout();
        break;
      case R.id.action_quit:
        quit();
        break;
      default:
        return super.onOptionsItemSelected(item);
    }
    return true;
  }
  
  @Override
  public void onServiceConnected(PlaybackService service) {
    this.service = service;
    for (Fragment f : getSupportFragmentManager().getFragments()) {
      if (f instanceof PlaybackServiceConnection.Callback) {
        ((PlaybackServiceConnection.Callback) f).onServiceConnected(service);
      }
    }
  }
  
  private void fillPages() { 
    final FragmentManager manager = getSupportFragmentManager();
    final FragmentTransaction transaction = manager.beginTransaction();
    if (null == manager.findFragmentById(R.id.now_playing)) {
      transaction.replace(R.id.now_playing, NowPlayingFragment.createInstance());
    }
    if (null == manager.findFragmentById(R.id.browser_view)) {
      transaction.replace(R.id.browser_view, BrowserFragment.createInstance());
    }
    if (null == manager.findFragmentById(R.id.playlist_view)) {
      transaction.replace(R.id.playlist_view, PlaylistFragment.createInstance());
    }
    PlaybackServiceConnection.register(manager, transaction);
    transaction.commit();
    final ViewPager pager = (ViewPager) findViewById(R.id.view_pager);
    if (null != pager) {
      setupViewPager(pager);
    }
  }
  
  private void setupViewPager(ViewPager pager) {
    final int childs = pager.getChildCount() - 1;
    pager.setOffscreenPageLimit(childs);
    final String[] titles = new String[childs];
    for (int i = 0; i != titles.length; ++i) {
      titles[i] = getTitle(pager.getChildAt(1 + i));
    }
    pager.setAdapter(new Adapter(titles));
  }
  
  private String getTitle(View pane) {
    //only String can be stored in View's tag, so try to decode manually
    final String ID_PREFIX = "@";
    final String tag = (String) pane.getTag();
    return tag.startsWith(ID_PREFIX)
      ? getStringByName(tag.substring(ID_PREFIX.length()))
      : tag;
  }
  
  private String getStringByName(String name) {
    final int id = getResources().getIdentifier(name, null, getPackageName());
    return getResources().getString(id);
  }
  
  private void showPreferences() {
    final Intent intent = new Intent(this, PreferencesActivity.class);
    startActivity(intent);
  }
  
  private void showAbout() {
    final DialogFragment fragment = AboutFragment.createInstance();
    fragment.show(getSupportFragmentManager(), "about");
  }
  
  private void quit() {
    if (service != null) {
      service.getPlaybackControl().stop();
      PlaybackServiceConnection.shutdown(getSupportFragmentManager());
    }
    finish();
  }
  
  private static class Adapter extends PagerAdapter {

    private final String[] titles;

    Adapter(String[] titles) {
      this.titles = titles;
    }

    @Override
    public int getCount() {
      return titles.length;
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
      return view == object;
    }

    @Override
    public Object instantiateItem(ViewGroup container, int position) {
      return container.getChildAt(1 + position);
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
    }
    
    @Override
    public CharSequence getPageTitle(int position) {
      return titles[position]; 
    }
  }
}
