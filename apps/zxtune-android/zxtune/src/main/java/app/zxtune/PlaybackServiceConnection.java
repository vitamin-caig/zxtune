/**
 *
 * @file
 *
 * @brief Persistent background service connection holder
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.stubs.PlaybackServiceStub;
import app.zxtune.rpc.IRemotePlaybackService;
import app.zxtune.rpc.PlaybackServiceClient;

public class PlaybackServiceConnection extends Fragment {

  public interface Callback {
    
    void onServiceConnected(PlaybackService svc);
  }
  
  private static final String TAG = PlaybackServiceConnection.class.getName();
  
  private ServiceConnection connection;
  private PlaybackService service;
  private Callback subscriber; 
    
  static void register(FragmentManager manager, FragmentTransaction transaction) {
    if (manager.findFragmentByTag(TAG) == null) {
      final Fragment self = new PlaybackServiceConnection();
      transaction.add(self, TAG);
    }
  }
  
  static void shutdown(FragmentManager manager) {
    final PlaybackServiceConnection self = (PlaybackServiceConnection) manager.findFragmentByTag(TAG);
    self.shutdownService();
  }
  
  private synchronized void shutdownService() {
    disconnect();
    final Context context = getAppContext();
    final Intent intent = new Intent(context, MainService.class);
    context.stopService(intent);
  }
  
  @Override
  public void onActivityCreated(Bundle savedInstanceState) {
    super.onActivityCreated(savedInstanceState);
    
    setRetainInstance(true);

    try {
      connect();
    } catch (Exception e) {
      Log.w(TAG, e, "onActivityCreated()");
    }
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
  
  private synchronized void connect() throws Exception {
    Log.d(TAG, "Connecting to service");
    final Context context = getAppContext();
    final Intent intent = MainService.createIntent(context, null);
    context.startService(intent);
    final ServiceConnection connection = new ServiceConnectionCallback();
    if (!context.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
      throw new Exception("Failed to bind to service");
    }
    this.connection = connection;
  }
  
  private synchronized void disconnect() {
    if (connection != null && service != null) {
      Log.d(TAG, "Disconnecting from service");
      final Context context = getAppContext();
      context.unbindService(connection);
      connection = null;
    }
  }
  
  private synchronized void setCallback(@Nullable Callback cb) {
    subscriber = cb;
    notifySubscriber();
  }
  
  private synchronized void setService(PlaybackService svc) {
    service = svc;
    notifySubscriber();
  }
  
  private void notifySubscriber() {
    if (subscriber != null && service != null) {
      subscriber.onServiceConnected(service);
    }
  }
  
  private Context getAppContext() {
    return getActivity().getApplicationContext();
  }
  
  private class ServiceConnectionCallback implements ServiceConnection {

    @Override
    public void onServiceConnected(ComponentName component, IBinder binder) {
      Log.d(TAG, "Connected!");
      final IRemotePlaybackService iface = IRemotePlaybackService.Stub.asInterface(binder);
      final PlaybackService service = new PlaybackServiceClient(iface);
      setService(service);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
      Log.d(TAG, "Disconnected!");
      setService(PlaybackServiceStub.instance());
    }
  }
}
