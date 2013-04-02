/*
 * @file
 * @brief Remote stub for Playback.Control
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.io.Closeable;
import java.io.IOException;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import app.zxtune.Playback;

public class PlaybackControlClient implements Playback.Control, Closeable {

  public interface ConnectionHandler {
    
    public void onConnected(PlaybackControlClient client);

    public void onDisconnected();
  }

  public static void create(Context context, Intent intent, ConnectionHandler handler) {
    context.startService(intent);
    final ServiceConnection svcHandler = new ServiceConnectionHandler(context, handler);
    if (!context.bindService(intent, svcHandler, Context.BIND_AUTO_CREATE)) {
      throw new RuntimeException("Failed to bind to service");
    }
  }

  private final static String TAG = PlaybackControlClient.class.getName();
  private final Context context;
  private final ServiceConnection handler;
  private final IPlaybackControl delegate;

  private PlaybackControlClient(Context context, ServiceConnection handler, IPlaybackControl delegate) {
    this.context = context;
    this.handler = handler;
    this.delegate = delegate;
  }
  
  @Override
  public void close() throws IOException {
    context.unbindService(handler);
  }

  @Override
  public Playback.Item getItem() {
    try {
      return delegate.getItem();
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public Playback.Status getStatus() {
    try {
      return delegate.getStatus();
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public void play(Uri item) {
    try {
      delegate.playItem(item);
    } catch (RemoteException e) {
    }
  }

  @Override
  public void play() {
    try {
      delegate.play();
    } catch (RemoteException e) {
    }
  }

  @Override
  public void pause() {
    try {
      delegate.pause();
    } catch (RemoteException e) {
    }
  }

  @Override
  public void stop() {
    try {
      delegate.stop();
    } catch (RemoteException e) {
    }
  }

  private static class ServiceConnectionHandler implements ServiceConnection {
    
    private final Context context;
    private final ConnectionHandler handler;
    
    public ServiceConnectionHandler(Context context, ConnectionHandler handler) {
      this.context = context;
      this.handler = handler;
    }

    @Override
    public void onServiceConnected(ComponentName className, IBinder service) {
      Log.d(TAG, "Connected to service");
      final IPlaybackControl delegate = IPlaybackControl.Stub.asInterface(service);
      final PlaybackControlClient client = new PlaybackControlClient(context, this, delegate);
      handler.onConnected(client);
    }

    @Override
    public void onServiceDisconnected(ComponentName className) {
      Log.d(TAG, "Disconnected from service");
      handler.onDisconnected();
    }
  };
}
