/**
 * @file
 * @brief Background playback service
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.MediaBrowserServiceCompat;
import android.support.v4.media.session.MediaButtonReceiver;
import android.text.TextUtils;
import app.zxtune.device.media.MediaSessionControl;
import app.zxtune.device.ui.StatusNotification;
import app.zxtune.device.ui.WidgetHandler;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.playback.service.PlayingStateCallback;
import app.zxtune.playback.stubs.CallbackStub;
import app.zxtune.rpc.PlaybackServiceServer;

import java.util.List;

public class MainService extends MediaBrowserServiceCompat {

  private static final String TAG = MainService.class.getName();

  public static final String CUSTOM_ACTION_ADD_CURRENT = TAG + ".CUSTOM_ACTION_ADD_CURRENT";
  public static final String CUSTOM_ACTION_ADD = TAG + ".CUSTOM_ACTION_ADD";

  private PlaybackServiceLocal service;
  private IBinder binder;

  private MediaSessionControl mediaSessionControl;
  private Releaseable settingsChangedHandler;

  public static Intent createIntent(Context ctx, @Nullable String action) {
    return new Intent(ctx, MainService.class).setAction(action);
  }

  @Override
  public void onCreate() {
    super.onCreate();
    Log.d(TAG, "Creating");

    service = new PlaybackServiceLocal(getApplicationContext());
    binder = new PlaybackServiceServer(service);

    setupCallbacks(getApplicationContext());
    setupServiceSessions();
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    Log.d(TAG, "Destroying");
    settingsChangedHandler.release();
    settingsChangedHandler = null;
    mediaSessionControl.release();
    mediaSessionControl = null;
    binder = null;
    service.release();
    service = null;
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "onStartCommand(%s)", intent);
    MediaButtonReceiver.handleIntent(mediaSessionControl.getSession(), intent);
    return super.onStartCommand(intent, flags, startId);
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind(%s)", intent.getAction());
    if (SERVICE_INTERFACE.equals(intent.getAction())) {
      return super.onBind(intent);
    } else {
      return binder;
    }
  }

  @Nullable
  @Override
  public BrowserRoot onGetRoot(@NonNull String clientPackageName, int clientUid, @Nullable Bundle rootHints) {
    Log.d(TAG, "onGetRoot(%s)", clientPackageName);
    if (TextUtils.equals(clientPackageName, getPackageName())) {
      return new BrowserRoot(getString(R.string.app_name), null);
    }

    return null;
  }

  //Not important for general audio service, required for class
  @Override
  public void onLoadChildren(@NonNull String parentId, @NonNull Result<List<MediaBrowserCompat.MediaItem>> result) {
    Log.d(TAG, "onLoadChildren(%s)", parentId);
    result.sendResult(null);
  }

  private void setupCallbacks(Context ctx) {
    //should be always paired
    mediaSessionControl = MediaSessionControl.subscribe(ctx, service);
    StatusNotification.connect(this, mediaSessionControl.getSession());
    setSessionToken(mediaSessionControl.getSession().getSessionToken());

    service.subscribe(new Analytics.PlaybackEventsCallback());
    service.subscribe(new PlayingStateCallback(ctx));

    WidgetHandler.connect(ctx, mediaSessionControl.getSession());

    settingsChangedHandler = ChangedSettingsReceiver.subscribe(ctx);
  }

  private void setupServiceSessions() {
    service.restoreSession();
    service.subscribe(new CallbackStub() {
      @Override
      public void onStateChanged(PlaybackControl.State state, TimeStamp pos) {
        if (state == PlaybackControl.State.STOPPED) {
          service.storeSession();
        }
      }
    });
  }
}
