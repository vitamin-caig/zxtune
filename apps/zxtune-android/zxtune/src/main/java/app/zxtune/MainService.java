/**
 * @file
 * @brief Background playback service
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.text.TextUtils;

import androidx.annotation.Nullable;
import androidx.media.MediaBrowserServiceCompat;
import androidx.media.session.MediaButtonReceiver;

import java.util.ArrayList;
import java.util.List;

import app.zxtune.analytics.Analytics;
import app.zxtune.core.PropertiesModifier;
import app.zxtune.core.jni.Api;
import app.zxtune.device.media.AudioFocusConnection;
import app.zxtune.device.media.MediaSessionControl;
import app.zxtune.device.media.NoisyAudioConnection;
import app.zxtune.device.ui.StatusNotification;
import app.zxtune.device.ui.WidgetHandler;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.preferences.Preferences;
import app.zxtune.preferences.SharedPreferencesBridge;

public class MainService extends MediaBrowserServiceCompat {

  private static final String TAG = MainService.class.getName();

  private static final Analytics.Trace TRACE = Analytics.Trace.create("MainService");

  public static final String CUSTOM_ACTION_ADD_CURRENT = TAG + ".CUSTOM_ACTION_ADD_CURRENT";
  public static final String CUSTOM_ACTION_ADD = TAG + ".CUSTOM_ACTION_ADD";

  private final ArrayList<Releaseable> handles = new ArrayList<>();
  @Nullable
  private PlaybackServiceLocal service;
  @Nullable
  private MediaSessionCompat session;

  public static Intent createIntent(Context ctx, @Nullable String action) {
    return new Intent(ctx, MainService.class).setAction(action);
  }

  @Override
  public void onCreate() {
    TRACE.beginMethod("onCreate");
    super.onCreate();
    TRACE.checkpoint("super");

    final Context ctx = getApplicationContext();
    SharedPreferences prefs = Preferences.getDefaultSharedPreferences(ctx);

    Api.load(() -> runOnMainThread(() -> onJniReady(prefs)));
    TRACE.checkpoint("jni");

    service = new PlaybackServiceLocal(ctx, Preferences.getDataStore(ctx));
    addHandle(service);
    TRACE.checkpoint("svc");

    setupCallbacks(ctx, service);
    TRACE.checkpoint("cbs");

    service.restoreSession();
    TRACE.endMethod();
  }

  // TODO: extract
  private static void runOnMainThread(Runnable r) {
    new Handler(Looper.getMainLooper()).post(r);
  }

  private void onJniReady(SharedPreferences prefs) {
    try {
      Log.d(TAG, "JNI is ready");
      PropertiesModifier options = Api.instance().getOptions();
      addHandle(SharedPreferencesBridge.subscribe(prefs, options));
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to connect to native options");
    }
  }

  private void addHandle(Releaseable r) {
    handles.add(r);
  }

  @Override
  public void onDestroy() {
    for (Releaseable r : handles) {
      r.release();
    }
    handles.clear();
    session = null;
    service = null;
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
    MediaButtonReceiver.handleIntent(session, intent);
    if (intent != null) {
      handleCommand(intent);
    }
    return super.onStartCommand(intent, flags, startId);
  }

  private void handleCommand(Intent intent) {
    if (CUSTOM_ACTION_ADD_CURRENT.equals(intent.getAction())) {
      session.getController().getTransportControls().sendCustomAction(CUSTOM_ACTION_ADD_CURRENT, null);
    }
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

  private void setupCallbacks(Context ctx, PlaybackServiceLocal service) {
    handles.add(AudioFocusConnection.create(ctx, service));
    handles.add(NoisyAudioConnection.create(ctx, service));
    //should be always paired
    MediaSessionControl ctrl = MediaSessionControl.subscribe(ctx, service);
    handles.add(ctrl);
    session = ctrl.getSession();
    handles.add(StatusNotification.connect(this, session));
    setSessionToken(session.getSessionToken());

    WidgetHandler.connect(ctx, session);
  }
}
