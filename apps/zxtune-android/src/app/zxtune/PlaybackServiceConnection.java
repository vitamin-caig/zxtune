/**
 * @file
 * @brief Fragment for storing CallbackSubscription
 * @version $Id:$
 * @author
 */
package app.zxtune;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.util.Log;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.PlaybackService;
import app.zxtune.rpc.BroadcastPlaybackCallback;
import app.zxtune.rpc.IRemotePlaybackService;
import app.zxtune.rpc.PlaybackServiceClient;

public class PlaybackServiceConnection extends Fragment {

  public interface Callback {
    
    public void onServiceConnected(PlaybackService svc);
  }
  
  private static final String TAG = PlaybackServiceConnection.class.getName();
  
  private ServiceConnection connection;
  private PlaybackService service;
  private Callback subscriber; 
    
  public static void register(FragmentManager manager, FragmentTransaction transaction) {
    if (manager.findFragmentByTag(TAG) == null) {
      final Fragment self = new PlaybackServiceConnection();
      transaction.add(self, TAG);
    }
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setRetainInstance(true);
    
    connect();
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);

    setCallback((Callback) activity);
  }
  
  @Override
  public void onDetach() {
    super.onDetach();

    setCallback(null);
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
    disconnect();
  }
  
  private synchronized void connect() {
    Log.d(TAG, "Connecting to service");
    final Context context = getContext();
    final Intent intent = new Intent(context, MainService.class);
    context.startService(intent);
    final ServiceConnection connection = new ServiceConnectionCallback();
    if (!context.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
      throw new RuntimeException("Failed to bind to service");
    }
    this.connection = connection;
  }
  
  private synchronized void disconnect() {
    if (connection != null) {
      Log.d(TAG, "Disconnecting from service");
      final Context context = getContext();
      context.unbindService(connection);
      connection = null;
    }
  }
  
  private synchronized void setCallback(Callback cb) {
    subscriber = cb;
    if (subscriber != null && service != null) {
      subscriber.onServiceConnected(service);
    }
  }
  
  private synchronized void setService(PlaybackService svc) {
    service = svc;
    if (subscriber != null) {
      subscriber.onServiceConnected(service);
    }
  }
  
  private Context getContext() {
    return getActivity().getApplicationContext();
  }
  
  private class ServiceConnectionCallback implements ServiceConnection {

    private Releaseable callbackConnection;
    
    @Override
    public void onServiceConnected(ComponentName component, IBinder binder) {
      Log.d(TAG, "Connected!");
      final IRemotePlaybackService iface = IRemotePlaybackService.Stub.asInterface(binder);
      final CompositeCallback callbacks = new CompositeCallback();
      final PlaybackService service = new PlaybackServiceClient(iface, callbacks);
      callbacks.onItemChanged(service.getNowPlaying());
      callbacks.onStatusChanged(service.getPlaybackControl().isPlaying());
      callbackConnection = BroadcastPlaybackCallback.createClientStub(getContext(), callbacks);
      setService(service);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
      Log.d(TAG, "Disconnected!");
      callbackConnection.release();
      callbackConnection = null;
      setService(null);//TODO: stub?
    }
  }
}
