package app.zxtune.net;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import androidx.annotation.Nullable;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class NetworkManager {

  public interface Callback {
    void onNetworkChange(boolean isAvailable);
  }

  @Nullable
  private static NetworkManager instance;

  private final CompositeCallback callbacks;
  private final NetworkStatusReceiver receiver;

  private NetworkManager(Context ctx) {
    this.callbacks = new CompositeCallback();
    this.receiver = new NetworkStatusReceiver(ctx, callbacks);
    ctx.registerReceiver(receiver,
        new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
  }

  public static void initialize(Context ctx) {
    instance = new NetworkManager(ctx);
  }

  public static NetworkManager getInstance() {
    return instance;
  }

  public final boolean isNetworkAvailable() {
    return receiver.isNetworkAvailable();
  }

  public final void subscribe(Callback cb) {
    callbacks.add(cb);
  }

  private static class CompositeCallback implements Callback {

    private final ArrayList<WeakReference<Callback>> delegates = new ArrayList<>(5);

    synchronized final void add(Callback cb) {
      delegates.add(new WeakReference<>(cb));
    }

    @Override
    synchronized public void onNetworkChange(boolean isActive) {
      for (int idx = 0, lim = delegates.size(); idx < lim; ) {
        final Callback cb = delegates.get(idx).get();
        if (cb != null) {
          cb.onNetworkChange(isActive);
          ++idx;
        } else {
          delegates.remove(idx);
          --lim;
        }
      }
    }
  }

  private static class NetworkStatusReceiver extends BroadcastReceiver {

    private final Callback cb;
    private boolean isNetworkAvailable;

    NetworkStatusReceiver(Context ctx, Callback cb) {
      this.cb = cb;
      this.isNetworkAvailable = isNetworkAvailable(ctx);
    }

    final boolean isNetworkAvailable() {
      return isNetworkAvailable;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
      if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
        final boolean newState = isNetworkAvailable(context);
        if (newState != isNetworkAvailable) {
          cb.onNetworkChange(isNetworkAvailable = newState);
        }
      }
    }

    private static boolean isNetworkAvailable(Context ctx) {
      final ConnectivityManager connectivityManager =
          (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
      final NetworkInfo info = connectivityManager != null ? connectivityManager.getActiveNetworkInfo() : null;
      return info != null && info.isConnected();
    }
  }
}
