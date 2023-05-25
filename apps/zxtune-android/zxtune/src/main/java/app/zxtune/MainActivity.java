/**
 * @file
 * @brief Main application activity
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.Manifest;
import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.v4.media.session.MediaControllerCompat;
import android.util.SparseArray;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.Observer;
import androidx.viewpager.widget.ViewPager;

import app.zxtune.analytics.Analytics;
import app.zxtune.device.Permission;
import app.zxtune.device.media.MediaModel;
import app.zxtune.ui.AboutActivity;
import app.zxtune.ui.ViewPagerAdapter;

public class MainActivity extends AppCompatActivity {

  public interface PagerTabListener {
    void onTabVisibilityChanged(boolean isVisible);
  }

  private static class StubPagerTabListener implements PagerTabListener {
    @Override
    public void onTabVisibilityChanged(boolean isVisible) {}

    public static PagerTabListener instance() {
      return Holder.INSTANCE;
    }

    //onDemand holder idiom
    private static class Holder {
      public static final PagerTabListener INSTANCE = new StubPagerTabListener();
    }

  }

  private static final Analytics.Trace TRACE = Analytics.Trace.create("MainActivity");
  @Nullable
  private ViewPager pager;
  public static final int PENDING_INTENT_FLAG = Build.VERSION.SDK_INT >= 23 ? PendingIntent.FLAG_IMMUTABLE : 0;

  public static PendingIntent createPendingIntent(Context ctx) {
    final Intent intent = new Intent(ctx, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    return PendingIntent.getActivity(ctx, 0, intent, PENDING_INTENT_FLAG);
  }

  public MainActivity() {
    super(R.layout.main_activity);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    TRACE.beginMethod(savedInstanceState == null ? "onCreate" : "onRecreate");
    super.onCreate(savedInstanceState);
    TRACE.checkpoint("super");

    fillPages();
    Permission.request(this, Manifest.permission.READ_EXTERNAL_STORAGE);
    Permission.request(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
    TRACE.checkpoint("perm");

    final LiveData<MediaControllerCompat> ctrl = MediaModel.of(this).getController();
    ctrl.observe(this, (@Nullable MediaControllerCompat update) -> {
      MediaControllerCompat.setMediaController(this, update);
    });

    if (savedInstanceState == null) {
      subscribeForPendingOpenRequest(ctrl);
    }
    Analytics.sendUiEvent(Analytics.UI_ACTION_OPEN);

    if (!BuildConfig.BUILD_TYPE.equals("release")) {
      StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder().detectAll().build());
      StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().detectAll().build());
    }

    TRACE.endMethod();
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    Analytics.sendUiEvent(Analytics.UI_ACTION_CLOSE);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    super.onCreateOptionsMenu(menu);
    getMenuInflater().inflate(R.menu.main, menu);
    menu.findItem(R.id.action_about).setIntent(AboutActivity.createIntent(this));
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.action_prefs:
        showPreferences();
        break;
      case R.id.action_rate:
        rateApplication();
        break;
      case R.id.action_quit:
        quit();
        break;
      default:
        return super.onOptionsItemSelected(item);
    }
    return true;
  }

  private void subscribeForPendingOpenRequest(LiveData<MediaControllerCompat> ctrl) {
    final Intent intent = getIntent();
    if (intent != null && Intent.ACTION_VIEW.equals(intent.getAction())) {
      final Uri uri = intent.getData();
      if (uri != null) {
        ctrl.observe(this, new Observer<MediaControllerCompat>() {
          @Override
          public void onChanged(@Nullable MediaControllerCompat mediaControllerCompat) {
            if (mediaControllerCompat != null) {
              mediaControllerCompat.getTransportControls().playFromUri(uri, null);
              ctrl.removeObserver(this);
            }
          }
        });
      }
    }
  }

  private void fillPages() {
    pager = findViewById(R.id.view_pager);
    if (null != pager) {
      final ViewPagerAdapter adapter = new ViewPagerAdapter(pager);
      pager.setAdapter(adapter);
      pager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {

        private final SparseArray<PagerTabListener> listeners = new SparseArray<>();
        private int prevPos = pager.getCurrentItem();

        @Override
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        }

        @Override
        public void onPageSelected(int newPos) {
          getListener(prevPos).onTabVisibilityChanged(false);
          getListener(newPos).onTabVisibilityChanged(true);
          prevPos = newPos;
        }

        @Override
        public void onPageScrollStateChanged(int state) {
        }

        private PagerTabListener getListener(int idx) {
          PagerTabListener result = listeners.get(idx);
          if (result == null) {
            result = createListener(idx);
            listeners.put(idx, result);
          }
          return result;
        }

        private PagerTabListener createListener(int idx) {
          final int id = ((View) adapter.instantiateItem(pager, idx)).getId();
          final Fragment frag = getSupportFragmentManager().findFragmentById(id);
          if (frag instanceof PagerTabListener) {
            return (PagerTabListener) frag;
          } else {
            return StubPagerTabListener.instance();
          }
        }
      });
    }
  }

  private void showPreferences() {
    final Intent intent = new Intent(this, PreferencesActivity.class);
    startActivity(intent);
    Analytics.sendUiEvent(Analytics.UI_ACTION_PREFERENCES);
  }

  private void rateApplication() {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse("market://details?id=" + getPackageName()));
    if (safeStartActivity(intent)) {
      Analytics.sendUiEvent(Analytics.UI_ACTION_RATE);
    } else {
      intent.setData(Uri.parse("https://play.google.com/store/apps/details?id=" + getPackageName()));
      if (safeStartActivity(intent)) {
        Analytics.sendUiEvent(Analytics.UI_ACTION_RATE);
      } else {
        Toast.makeText(this, "Error", Toast.LENGTH_LONG).show();
      }
    }
  }

  private boolean safeStartActivity(Intent intent) {
    try {
      startActivity(intent);
      return true;
    } catch (ActivityNotFoundException e) {
      return false;
    }
  }

  private void quit() {
    Analytics.sendUiEvent(Analytics.UI_ACTION_QUIT);
    final Intent intent = MainService.createIntent(this, null);
    stopService(intent);
    finish();
  }
}
