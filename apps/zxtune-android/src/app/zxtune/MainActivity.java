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
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.ViewPager;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import app.zxtune.playback.PlaybackService;
import app.zxtune.ui.AboutFragment;
import app.zxtune.ui.BrowserFragment;
import app.zxtune.ui.NowPlayingFragment;
import app.zxtune.ui.PlaylistFragment;
import app.zxtune.ui.ViewPagerAdapter;

public class MainActivity extends ActionBarActivity implements PlaybackServiceConnection.Callback {
  
  private static final int NO_PAGE = -1;
  private PlaybackService service;
  private ViewPager pager;
  private int browserPageIndex;
  private BrowserFragment browser;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    supportRequestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
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
  public void onBackPressed() {
    if (pager != null && pager.getCurrentItem() == browserPageIndex) {
      browser.moveUp();
    } else {
      super.onBackPressed();
    }
  }
  
  @Override
  public void onDestroy() {
    browser = null;
    pager = null;
    super.onDestroy();
  }
  
  @Override
  public void onServiceConnected(PlaybackService service) {
    this.service = service;
    for (Fragment f : getSupportFragmentManager().getFragments()) {
      if (f instanceof PlaybackServiceConnection.Callback) {
        ((PlaybackServiceConnection.Callback) f).onServiceConnected(service);
      }
    }
    redirectIntentData();
  }
  
  private void redirectIntentData() {
    final Intent intent = getIntent();
    if (intent.getAction().equals(Intent.ACTION_VIEW)) {
      final Uri[] path = {intent.getData()};
      service.setNowPlaying(path);
    }
  }
  
  private void fillPages() { 
    final FragmentManager manager = getSupportFragmentManager();
    final FragmentTransaction transaction = manager.beginTransaction();
    if (null == manager.findFragmentById(R.id.now_playing)) {
      transaction.replace(R.id.now_playing, NowPlayingFragment.createInstance());
    }
    browser = (BrowserFragment) manager.findFragmentById(R.id.browser_view);
    if (null == browser) {
      browser = BrowserFragment.createInstance();
      transaction.replace(R.id.browser_view, browser);
    }
    if (null == manager.findFragmentById(R.id.playlist_view)) {
      transaction.replace(R.id.playlist_view, PlaylistFragment.createInstance());
    }
    PlaybackServiceConnection.register(manager, transaction);
    transaction.commit();
    setupViewPager();
  }
  
  private void setupViewPager() {
    pager = (ViewPager) findViewById(R.id.view_pager);
    if (null != pager) {
      final ViewPagerAdapter adapter = new ViewPagerAdapter(pager); 
      pager.setAdapter(adapter);
      browserPageIndex = adapter.getCount() - 1;
      while (browserPageIndex >= 0 && !hasBrowserView(adapter.instantiateItem(pager, browserPageIndex))) {
        --browserPageIndex;
      }
    } else {
      browserPageIndex = NO_PAGE;
    }
  }
  
  private static boolean hasBrowserView(Object view) {
    return ((View) view).findViewById(R.id.browser_view) != null;
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
}
