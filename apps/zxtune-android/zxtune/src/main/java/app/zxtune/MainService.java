/**
 * @file
 * @brief Background playback service
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.media.MediaBrowserCompat;
import android.text.TextUtils;

import androidx.annotation.Nullable;
import androidx.media.MediaBrowserServiceCompat;
import androidx.media.session.MediaButtonReceiver;

import java.util.List;

import app.zxtune.device.media.MediaSessionControl;
import app.zxtune.device.ui.StatusNotification;
import app.zxtune.device.ui.WidgetHandler;
import app.zxtune.playback.service.PlaybackServiceLocal;

public class MainService extends MediaBrowserServiceCompat {

  private static final String TAG = MainService.class.getName();

  public static final String CUSTOM_ACTION_ADD_CURRENT = TAG + ".CUSTOM_ACTION_ADD_CURRENT";
  public static final String CUSTOM_ACTION_ADD = TAG + ".CUSTOM_ACTION_ADD";

  @Nullable
  private PlaybackServiceLocal service;
  @Nullable
  private MediaSessionControl mediaSessionControl;
  @Nullable
  private Releaseable settingsChangedHandler;

  public static Intent createIntent(Context ctx, @Nullable String action) {
    return new Intent(ctx, MainService.class).setAction(action);
  }

  @Override
  public void onCreate() {
    super.onCreate();
    Log.d(TAG, "Creating");

    final Context ctx = getApplicationContext();
    //Should be first call in order to initialize JNI and avoid deadlocks in Runtime
    settingsChangedHandler = ChangedSettingsReceiver.subscribe(ctx);
    service = new PlaybackServiceLocal(ctx);

    setupCallbacks(ctx);

    service.restoreSession();
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    mediaSessionControl.release();
    mediaSessionControl = null;
    service.release();
    service = null;
    settingsChangedHandler.release();
    settingsChangedHandler = null;
    super.onDestroy();
  }

  @Override
  public void onTaskRemoved(Intent rootIntent) {
    super.onTaskRemoved(rootIntent);
    service.getPlaybackControl().stop();
    stopSelf();
  }

  @Override
  public int onStartCommand(@Nullable Intent intent, int flags, int startId) {
    Log.d(TAG, "onStartCommand(%s)", intent);
    MediaButtonReceiver.handleIntent(mediaSessionControl.getSession(), intent);
    return super.onStartCommand(intent, flags, startId);
  }

  @Nullable
  @Override
  public BrowserRoot onGetRoot(String clientPackageName, int clientUid, @Nullable Bundle rootHints) {
    Log.d(TAG, "onGetRoot(%s)", clientPackageName);
    if (TextUtils.equals(clientPackageName, getPackageName())) {
      return new BrowserRoot(getString(R.string.app_name), null);
    }

    return null;
  }

  //Not important for general audio service, required for class
  @Override
  public void onLoadChildren(String parentId, Result<List<MediaBrowserCompat.MediaItem>> result) {
    Log.d(TAG, "onLoadChildren(%s)", parentId);
    result.sendResult(null);
  }

  private void setupCallbacks(Context ctx) {
    //should be always paired
    mediaSessionControl = MediaSessionControl.subscribe(ctx, service);
    StatusNotification.connect(this, mediaSessionControl.getSession());
    setSessionToken(mediaSessionControl.getSession().getSessionToken());

    WidgetHandler.connect(ctx, mediaSessionControl.getSession());
  }
}
