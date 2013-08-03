/*
 * @file
 * @brief Background service class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.IOException;

import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.playback.FileIterator;
import app.zxtune.playback.PlaybackServiceLocal;
import app.zxtune.playlist.Query;
import app.zxtune.rpc.BroadcastPlaybackCallback;
import app.zxtune.rpc.PlaybackServiceServer;
import app.zxtune.ui.StatusNotification;

public class MainService extends Service {

  private final static String TAG = MainService.class.getName();

  private PlaybackServiceLocal service;
  private IBinder binder;
  private PhoneCallHandler callHandler;
  
  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");

    service = new PlaybackServiceLocal(getApplicationContext());
    final Intent intent = new Intent(this, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    service.subscribeForEvents(new StatusNotification(this, intent));
    service.subscribeForEvents(BroadcastPlaybackCallback.createServerStub(getApplicationContext()));
    binder = new PlaybackServiceServer(service);
    callHandler = new PhoneCallHandler(this, service.getPlaybackControl());
    callHandler.register();
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    callHandler.unregister();
    callHandler = null;
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
      final ZXTune.Module module = FileIterator.loadModule(uri);
      final app.zxtune.playlist.Item item = new app.zxtune.playlist.Item(uri, module);
      module.release();
      getContentResolver().insert(Query.unparse(null), item.toContentValues());
    } catch (IOException e) {
      Log.d(TAG, e.toString());
    }
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }
}
