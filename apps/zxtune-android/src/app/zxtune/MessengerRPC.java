/*
 * @file
 * 
 * @brief Messenger-based service control RPC
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

public final class MessengerRPC {

  protected final static int MSG_OPEN = 1;
  protected final static int MSG_PLAY = 2;
  protected final static int MSG_PAUSE = 3;
  protected final static int MSG_STOP = 4;
  protected final static String KEY_MODULEID = "module_id";

  static class ControlClient implements Playback.Control {

    private static String TAG = "app.zxtune.MessengerRPC.Client";

    private Context context;
    private final ServiceConnection handler = new ConnectionHandler();
    private Messenger service;

    public static Playback.Control create(Context ctx) {
      return new ControlClient(ctx);
    }

    public static void destroy(Playback.Control control) {
      final ControlClient client = (ControlClient) control;
      client.context.unbindService(client.handler);
    }

    private ControlClient(Context ctx) {
      this(ctx, new Intent(ctx, PlaybackService.class));
    }

    private ControlClient(Context ctx, Intent intent) {
      this.context = ctx;
      context.startService(intent);
      if (context.bindService(intent, handler, Context.BIND_AUTO_CREATE)) {
        Log.d(TAG, "Bound to service");
      }
    }

    public void open(String moduleId) {
      final Bundle bundle = new Bundle();
      bundle.putString(KEY_MODULEID, moduleId);
      final Message msg = Message.obtain(null, MSG_OPEN, bundle);
      sendMessage(msg);
    }

    public void play() {
      final Message msg = Message.obtain(null, MSG_PLAY);
      sendMessage(msg);
    }

    public void pause() {
      final Message msg = Message.obtain(null, MSG_PAUSE);
      sendMessage(msg);
    }

    public void stop() {
      final Message msg = Message.obtain(null, MSG_STOP);
      sendMessage(msg);
    }

    private void sendMessage(Message msg) {
      try {
        service.send(msg);
      } catch (RemoteException e) {
        Log.d(TAG, e.getMessage());
      }
    }

    private class ConnectionHandler implements ServiceConnection {

      public void onServiceConnected(ComponentName className, IBinder svc) {
        Log.d(TAG, "Connected to service");
        service = new Messenger(svc);
      }

      public void onServiceDisconnected(ComponentName className) {
        Log.d(TAG, "Disconnected from service");
        service = null;
      }
    }
  }

  static class ControlServer extends Handler {

    private Playback.Control control;

    public static IBinder createBinder(Playback.Control ctrl) {
      return new Messenger(new ControlServer(ctrl)).getBinder();
    }

    private ControlServer(Playback.Control control) {
      this.control = control;
    }

    @Override
    public void handleMessage(Message msg) {
      switch (msg.what) {
        case MSG_OPEN:
          control.open((String) ((Bundle) msg.obj).getString(KEY_MODULEID));
          break;
        case MSG_PLAY:
          control.play();
          break;
        case MSG_PAUSE:
          control.pause();
          break;
        case MSG_STOP:
          control.stop();
          break;
        default:
          super.handleMessage(msg);
      }
    }
  }
}
