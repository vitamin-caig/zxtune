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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

public final class MessengerRPC {
  
  protected static final class Messages {
    public static final int PLAY = 1;
    public static final int PAUSE = PLAY + 1; 
    public static final int STOP = PAUSE + 1;
    public static final int REGISTER_CALLBACK = STOP + 1;
    public static final int UNREGISTER_CALLBACK = REGISTER_CALLBACK + 1;
    public static final int STARTED = UNREGISTER_CALLBACK + 1;
    public static final int PAUSED = STARTED + 1;
    public static final int STOPPED = PAUSED + 1;
    public static final int POSITION_CHANGED = STOPPED + 1;
  }

  protected final static String KEY_DESCRIPTION = "description";
  protected final static String KEY_TIME = "time";

  static class ControlClient implements Playback.Control {

    private static String TAG = "app.zxtune.MessengerRPC.ControlClient";

    private Context context;
    private final ServiceConnection handler = new ConnectionHandler();
    private final MessagesQueue messages = new MessagesQueue();
    private Map<Playback.Callback, Messenger> callbacks =
        new HashMap<Playback.Callback, Messenger>();

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

    public void play() {
      final Message msg = Message.obtain(null, Messages.PLAY);
      messages.send(msg);
    }

    public void pause() {
      final Message msg = Message.obtain(null, Messages.PAUSE);
      messages.send(msg);
    }

    public void stop() {
      final Message msg = Message.obtain(null, Messages.STOP);
      messages.send(msg);
    }

    public void registerCallback(Playback.Callback cb) {
      final Messenger messenger = CallbackServer.createMessenger(cb);
      final Message msg = Message.obtain(null, Messages.REGISTER_CALLBACK);
      msg.replyTo = messenger;
      messages.send(msg);
      callbacks.put(cb, messenger);
    }

    public void unregisterCallback(Playback.Callback cb) {
      final Messenger messenger = callbacks.get(cb);
      if (messenger != null) {
        final Message msg = Message.obtain(null, Messages.UNREGISTER_CALLBACK);
        msg.replyTo = messenger;
        messages.send(msg);
        callbacks.remove(cb);
      } else {
        Log.d(TAG, "Callback is not registered");
      }
    }

    private class ConnectionHandler implements ServiceConnection {

      public void onServiceConnected(ComponentName className, IBinder svc) {
        Log.d(TAG, "Connected to service");
        messages.connected(new Messenger(svc));
      }

      public void onServiceDisconnected(ComponentName className) {
        Log.d(TAG, "Disconnected from service");
        messages.disconnected();
      }
    }

    private class MessagesQueue {

      private ArrayList<Message> messages = new ArrayList<Message>();
      private Messenger delegate;

      void send(Message msg) {
        try {
          synchronized (messages) {
            if (delegate != null) {
              delegate.send(msg);
            } else {
              messages.add(msg);
            }
          }
        } catch (RemoteException e) {
          Log.d(TAG, e.getMessage());
        }
      }

      void connected(Messenger messenger) {
        try {
          synchronized (messages) {
            delegate = messenger;
            for (Message msg : messages) {
              delegate.send(msg);
            }
            messages.clear();
          }
        } catch (RemoteException e) {
          Log.d(TAG, e.getMessage());
        }
      }

      void disconnected() {
        synchronized (messages) {
          delegate = null;
        }
      }
    }
  }

  static class CallbackClient implements Playback.Callback {

    private static String TAG = "app.zxtune.MessengerRPC.CallbackClient";

    private Messenger callback;

    static Playback.Callback create(Messenger callback) {
      return new CallbackClient(callback);
    }

    private CallbackClient(Messenger callback) {
      this.callback = callback;
    }

    public void started(String description, int duration) {
      final Bundle bundle = new Bundle();
      bundle.putString(KEY_DESCRIPTION, description);
      final Message msg = Message.obtain(null, Messages.STARTED, duration, 0, bundle);
      sendMessage(msg);
    }

    public void paused(String description) {
      final Bundle bundle = new Bundle();
      bundle.putString(KEY_DESCRIPTION, description);
      final Message msg = Message.obtain(null, Messages.PAUSED, bundle);
      sendMessage(msg);
    }

    public void stopped() {
      final Message msg = Message.obtain(null, Messages.STOPPED);
      sendMessage(msg);
    }

    public void positionChanged(int curFrame, String curTime) {
      final Bundle bundle = new Bundle();
      bundle.putString(KEY_TIME, curTime);
      final Message msg = Message.obtain(null, Messages.POSITION_CHANGED, curFrame, 0, bundle);
      sendMessage(msg);
    }

    private void sendMessage(Message msg) {
      try {
        callback.send(msg);
      } catch (RemoteException e) {
        Log.d(TAG, e.getMessage());
      }
    }
  }

  static class ControlServer extends Handler {

    private static String TAG = "app.zxtune.MessengerRPC.ControlServer";

    private Playback.Control control;
    private Map<Messenger, Playback.Callback> callbacks =
        new HashMap<Messenger, Playback.Callback>();

    public static IBinder createBinder(Playback.Control ctrl) {
      return new Messenger(new ControlServer(ctrl)).getBinder();
    }

    private ControlServer(Playback.Control control) {
      this.control = control;
    }

    @Override
    public void handleMessage(Message msg) {
      switch (msg.what) {
        case Messages.PLAY:
          control.play();
          break;
        case Messages.PAUSE:
          control.pause();
          break;
        case Messages.STOP:
          control.stop();
          break;
        case Messages.REGISTER_CALLBACK: {
          final Playback.Callback cb = CallbackClient.create(msg.replyTo);
          control.registerCallback(cb);
          callbacks.put(msg.replyTo, cb);
        }
          break;
        case Messages.UNREGISTER_CALLBACK: {
          final Playback.Callback cb = callbacks.get(msg.replyTo);
          if (cb != null) {
            control.unregisterCallback(cb);
            callbacks.remove(msg.replyTo);
          } else {
            Log.d(TAG, "Callback is not registered");
          }
        }
          break;
        default:
          super.handleMessage(msg);
      }
    }
  }

  static class CallbackServer extends Handler {

    private Playback.Callback callback;

    public static Messenger createMessenger(Playback.Callback cb) {
      return new Messenger(new CallbackServer(cb));
    }

    private CallbackServer(Playback.Callback callback) {
      this.callback = callback;
    }

    @Override
    public void handleMessage(Message msg) {
      final Bundle bundle = (Bundle) msg.obj;
      switch (msg.what) {
        case Messages.STARTED:
          callback.started(bundle.getString(KEY_DESCRIPTION), msg.arg1);
          break;
        case Messages.PAUSED:
          callback.paused(bundle.getString(KEY_DESCRIPTION));
          break;
        case Messages.STOPPED:
          callback.stopped();
          break;
        case Messages.POSITION_CHANGED:
          callback.positionChanged(msg.arg1, bundle.getString(KEY_TIME));
          break;
        default:
          super.handleMessage(msg);
      }
    }
  }
}
