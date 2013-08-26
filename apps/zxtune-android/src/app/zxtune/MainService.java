/*
 * @file
 * @brief Background service class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.IOException;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import app.zxtune.playback.FileIterator;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackServiceLocal;
import app.zxtune.playlist.Query;
import app.zxtune.rpc.PlaybackServiceServer;
import app.zxtune.ui.StatusNotification;

public class MainService extends Service {

  private final static String TAG = MainService.class.getName();

  private String PREF_MEDIABUTTONS;
  private boolean PREF_MEDIABUTTONS_DEFAULT;
  private String PREF_UNPLUGGING;
  private boolean PREF_UNPLUGGING_DEFAULT;

  private PlaybackServiceLocal service;
  private IBinder binder;
  private Releaseable phoneCallHandler;
  private Releaseable mediaButtonsHandler;
  private Releaseable headphonesPlugHandler;
  private Releaseable settingsChangedHandler;

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");

    final Resources resources = getResources();
    PREF_MEDIABUTTONS = resources.getString(R.string.pref_control_headset_mediabuttons);
    PREF_MEDIABUTTONS_DEFAULT =
        resources.getBoolean(R.bool.pref_control_headset_mediabuttons_default);
    PREF_UNPLUGGING = resources.getString(R.string.pref_control_headset_unplugging);
    PREF_UNPLUGGING_DEFAULT = resources.getBoolean(R.bool.pref_control_headset_unplugging_default);
    mediaButtonsHandler = ReleaseableStub.instance();
    headphonesPlugHandler = ReleaseableStub.instance();
    
    service = new PlaybackServiceLocal(getApplicationContext());
    final Intent intent = new Intent(this, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    service.subscribe(new StatusNotification(this, intent));
    binder = new PlaybackServiceServer(service);
    final PlaybackControl control = service.getPlaybackControl();
    phoneCallHandler = PhoneCallHandler.subscribe(this, control);
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
    connectMediaButtons(prefs.getBoolean(PREF_MEDIABUTTONS, PREF_MEDIABUTTONS_DEFAULT));
    connectHeadphonesPlugging(prefs.getBoolean(PREF_UNPLUGGING, PREF_UNPLUGGING_DEFAULT));
    settingsChangedHandler =
        new BroadcastReceiverConnection(this, new ChangedSettingsReceiver(), new IntentFilter(
            PreferencesActivity.ACTION_PREFERENCE_CHANGED));
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    settingsChangedHandler.release();
    settingsChangedHandler = null;
    headphonesPlugHandler.release();
    headphonesPlugHandler = null;
    mediaButtonsHandler.release();
    mediaButtonsHandler = null;
    phoneCallHandler.release();
    phoneCallHandler = null;
    binder = null;
    service.release();
    service = null;
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "StartCommand called");
    final String action = intent != null ? intent.getAction() : null;
    final Uri uri = intent != null ? intent.getData() : Uri.EMPTY;
    if (action != null && uri != Uri.EMPTY) {
      startAction(action, uri);
    }
    return START_NOT_STICKY;
  }

  private final void startAction(String action, Uri uri) {
    if (action.equals(Intent.ACTION_VIEW)) {
      Log.d(TAG, "Playing module " + uri);
      service.setNowPlaying(uri);
    } else if (action.equals(Intent.ACTION_INSERT)) {
      Log.d(TAG, "Adding to playlist all modules from " + uri);
      addModuleToPlaylist(uri);
    }
  }

  private void addModuleToPlaylist(Uri uri) {
    try {
      final FileIterator iter = new FileIterator(getApplicationContext(), uri);
      do {
        final PlayableItem item = iter.getItem();
        try {
          final app.zxtune.playlist.Item listItem =
              new app.zxtune.playlist.Item(uri, item.getModule());
          getContentResolver().insert(Query.unparse(null), listItem.toContentValues());
        } finally {
          item.release();
        }
      } while (iter.next());
    } catch (Error e) {
      Log.w(TAG, "addModuleToPlaylist()", e);
    }
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }

  private void connectMediaButtons(boolean connect) {
    Log.d(TAG, "connectMediaButtons = " + connect);
    final boolean connected = mediaButtonsHandler != ReleaseableStub.instance();
    if (connect != connected) {
      mediaButtonsHandler.release();
      mediaButtonsHandler =
          connect
              ? MediaButtonsHandler.subscribe(this, service.getPlaybackControl())
              : ReleaseableStub.instance();
    }
  }

  private void connectHeadphonesPlugging(boolean connect) {
    Log.d(TAG, "connectHeadPhonesPluggins = " + connect);
    final boolean connected = headphonesPlugHandler != ReleaseableStub.instance();
    if (connect != connected) {
      headphonesPlugHandler.release();
      headphonesPlugHandler =
          connect
              ? HeadphonesPlugHandler.subscribe(this, service.getPlaybackControl())
              : ReleaseableStub.instance();
    }
  }

  private class ChangedSettingsReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
      final String key = intent.getStringExtra(PreferencesActivity.EXTRA_PREFERENCE_NAME);
      if (key.equals(PREF_MEDIABUTTONS)) {
        final boolean use =
            intent.getBooleanExtra(PreferencesActivity.EXTRA_PREFERENCE_VALUE,
                PREF_MEDIABUTTONS_DEFAULT);
        connectMediaButtons(use);
      } else if (key.equals(PREF_UNPLUGGING)) {
        final boolean use =
            intent.getBooleanExtra(PreferencesActivity.EXTRA_PREFERENCE_VALUE,
                PREF_UNPLUGGING_DEFAULT);
        connectHeadphonesPlugging(use);
      }
    }
  }
}
