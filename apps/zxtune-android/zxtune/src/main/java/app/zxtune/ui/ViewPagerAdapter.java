/**
 * @file
 * @brief PagerAdapter implementation over layout-defined ViewPager content
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.PagerTabStrip;
import androidx.viewpager.widget.ViewPager;

public class ViewPagerAdapter extends PagerAdapter {

  private final int delta;
  private final String[] titles;

  public ViewPagerAdapter(ViewPager pager) {
    final View tabStrip = pager.getChildAt(0);
    if (tabStrip instanceof PagerTabStrip) {
      // This is actual for Sony XPeria https://issuetracker.google.com/issues/37105263
      ((ViewPager.LayoutParams) tabStrip.getLayoutParams()).isDecor = true;
      this.delta = 1;
    } else {
      this.delta = 0;
    }
    this.titles = new String[pager.getChildCount() - delta];
    for (int i = 0; i != titles.length; ++i) {
      titles[i] = getTitle(pager.getContext(), pager.getChildAt(delta + i));
    }
    pager.setOffscreenPageLimit(titles.length);
  }

  private String getTitle(Context ctx, View pane) {
    //only String can be stored in View's tag, so try to decode manually
    final String ID_PREFIX = "@";
    final String tag = (String) pane.getTag();
    return tag.startsWith(ID_PREFIX) ? getStringByName(ctx, tag.substring(ID_PREFIX.length())) : tag;
  }

  private String getStringByName(Context ctx, String name) {
    final int id = ctx.getResources().getIdentifier(name, null, ctx.getPackageName());
    return ctx.getString(id);
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
    return container.getChildAt(delta + position);
  }

  @Override
  public void destroyItem(ViewGroup container, int position, Object object) {
  }

  @Override
  public CharSequence getPageTitle(int position) {
    return titles[position];
  }
}
