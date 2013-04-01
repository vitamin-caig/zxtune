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

  private final static String TAG = PlaybackControlClient.class.getName();
  private final Context context;
  private final ServiceConnection handler = new ConnectionHandler();
  private IPlaybackControl delegate;

  public PlaybackControlClient(Context context, Intent intent) {
    this.context = context;
    this.context.startService(intent);
    if (context.bindService(intent, handler, Context.BIND_AUTO_CREATE)) {
      Log.d(TAG, "Bound to service");
    }
  }

  @Override
  public void close() throws IOException {
    context.unbindService(handler);
  }

  @Override
  public Playback.Item getItem() {
    if (null == delegate) {
      return null;
    }
    try {
      return delegate.getItem();
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public Playback.Status getStatus() {
    if (null == delegate) {
      return null;
    }
    try {
      return delegate.getStatus();
    } catch (RemoteException e) {
      return null;
    }
  }

  @Override
  public void play(Uri item) {
    if (null == delegate) {
      return;
    }
    try {
      delegate.playItem(item);
    } catch (RemoteException e) {
    }
  }

  @Override
  public void play() {
    if (null == delegate) {
      return;
    }
    try {
      delegate.play();
    } catch (RemoteException e) {
    }
  }

  @Override
  public void pause() {
    if (null == delegate) {
      return;
    }
    try {
      delegate.pause();
    } catch (RemoteException e) {
    }
  }

  @Override
  public void stop() {
    if (null == delegate) {
      return;
    }
    try {
      delegate.stop();
    } catch (RemoteException e) {
    }
  }

  class ConnectionHandler implements ServiceConnection {
    @Override
    public void onServiceConnected(ComponentName className, IBinder service) {
      delegate = IPlaybackControl.Stub.asInterface(service);
      Log.d(TAG, "Connected to service");
    }

    @Override
    public void onServiceDisconnected(ComponentName className) {
      delegate = null;
      Log.d(TAG, "Disconnected from service");
    }
  };
}
