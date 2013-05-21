/**
 * @file
 * @brief Playback control interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */
package app.zxtune.rpc;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;

public final class RemoteSubscription implements CallbackSubscription {
  
  private final Context context;
  private final Intent intent;
  
  public RemoteSubscription(Context context, Intent intent) {
    this.context = context;
    this.intent = intent;
  }
  
  @Override
  public Releaseable subscribe(Callback callback) {
    context.startService(intent);
    final ControlConnection connection = new ControlConnection(context, callback);
    if (!context.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
      throw new RuntimeException("Failed to bind to service");
    }
    return connection;
  }
  
  private static class ControlConnection extends BroadcastReceiver implements ServiceConnection, Releaseable {

    private final static String TAG = ControlConnection.class.getName();
    private final Context context;
    private final Callback callback;

    public ControlConnection(Context context, Callback callback) {
      this.context = context;
      this.callback = callback;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
      final String action = intent.getAction();
      if (action.equals(Status.class.getName())) {
        final boolean isPlaying = intent.getBooleanExtra(Status.class.getSimpleName(), false);
        callback.onStatusChanged(isPlaying);
      } else if (action.equals(Item.class.getName())) {
        final Item item = (Item) intent.getParcelableExtra(Item.class.getSimpleName());
        callback.onItemChanged(item);
      }
    }
    
    @Override
    public void onServiceConnected(ComponentName className, IBinder service) {
      Log.d(TAG, "Connected to service");
      final IPlaybackControl delegate = IPlaybackControl.Stub.asInterface(service);
      final Control control = new PlaybackControlClient(delegate);
      callback.onControlChanged(control);
      setupBroadcastConnections();
    }

    @Override
    public void onServiceDisconnected(ComponentName className) {
      Log.d(TAG, "Disconnected from service");
      release();
    }

    @Override
    public void release() {
      Log.d(TAG, "Closing connection to service");
      callback.onControlChanged(null);
      shutdownBroadcastConnections();
      context.unbindService(this);
    }
    
    private void setupBroadcastConnections() {
      final IntentFilter filter = new IntentFilter();
      filter.addAction(Item.class.getName());
      filter.addAction(Status.class.getName());
      context.registerReceiver(this, filter);
    }
    
    private void shutdownBroadcastConnections() {
      context.unregisterReceiver(this);
    }
  }
}
