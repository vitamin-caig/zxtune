/*
 * @file
 * @brief Currently playing activity
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.ui.BrowserFragment;
import app.zxtune.ui.NowPlayingFragment;
import app.zxtune.ui.PlaylistFragment;

public class MainActivity extends FragmentActivity {

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main_activity);

    fillPages();
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
    RetainedCallbackSubscriptionFragment.register(manager, transaction);
    transaction.commit();
    final ViewPager pager = (ViewPager) findViewById(R.id.view_pager);
    final int childs = pager.getChildCount();
    pager.setOffscreenPageLimit(childs);
    pager.setAdapter(new Adapter(childs));
  }
  
  private static class Adapter extends PagerAdapter {

    private final int count;

    public Adapter(int count) {
      this.count = count;
    }

    @Override
    public int getCount() {
      return count;
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
      return view == object;
    }

    @Override
    public Object instantiateItem(ViewGroup container, int position) {
      return container.getChildAt(position);
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
    }
  }
}
