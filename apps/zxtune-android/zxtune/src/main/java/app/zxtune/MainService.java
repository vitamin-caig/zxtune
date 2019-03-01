/**
 * @file
 * @brief Background playback service
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.media.session.MediaButtonReceiver;
import app.zxtune.device.sound.AudioFocusHandler;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.playback.service.PlayingStateCallback;
import app.zxtune.playback.stubs.CallbackStub;
import app.zxtune.rpc.PlaybackServiceServer;
import app.zxtune.ui.StatusNotification;

public class MainService extends Service {

  private static final String TAG = MainService.class.getName();

  private PlaybackServiceLocal service;
  private IBinder binder;

  private RemoteControl remoteControl;
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
    remoteControl.release();
    remoteControl = null;
    binder = null;
    service.release();
    service = null;
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "StartCommand called");
    MediaButtonReceiver.handleIntent(remoteControl.getSession(), intent);
    return super.onStartCommand(intent, flags, startId);
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }

  private void setupCallbacks(Context ctx) {
    //should be always paired
    service.subscribe(new AudioFocusHandler(ctx, service.getPlaybackControl()));
    remoteControl = RemoteControl.subscribe(ctx, service);

    service.subscribe(new Analytics.PlaybackEventsCallback());
    service.subscribe(new PlayingStateCallback(ctx));
    service.subscribe(new WidgetHandler.WidgetNotification(ctx));

    service.subscribe(new StatusNotification(this, remoteControl.getSession()));
    settingsChangedHandler = ChangedSettingsReceiver.subscribe(ctx);
  }

  private void setupServiceSessions() {
    service.restoreSession();
    service.subscribe(new CallbackStub() {
      @Override
      public void onStateChanged(PlaybackControl.State state) {
        if (state == PlaybackControl.State.STOPPED) {
          service.storeSession();
        }
      }
    });
  }
}
