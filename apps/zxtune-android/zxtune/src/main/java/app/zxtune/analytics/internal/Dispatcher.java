package app.zxtune.analytics.internal;

import android.content.Context;

import java.io.IOException;

import app.zxtune.Log;
import app.zxtune.net.NetworkManager;

class Dispatcher implements UrlsSink, NetworkManager.Callback {

  private static final String TAG = Dispatcher.class.getName();

  private static final int NETWORK_RETRIES_PERIOD = 1000;

  private final NetworkSink network;
  private final StubSink stub;
  private UrlsSink current;
  private int networkRetryCountdown;

  Dispatcher(Context ctx) {
    this.network = new NetworkSink(ctx);
    this.stub = new StubSink();
    NetworkManager.initialize(ctx);
    NetworkManager.getInstance().subscribe(this);
    onNetworkChange(NetworkManager.getInstance().isNetworkAvailable());
  }

  @Override
  public void push(String url) {
    try {
      current.push(url);
      trySwitchToNetwork();
    } catch (IOException e) {
      Log.w(TAG, e, "Network error, drop events");
      current = stub;
      networkRetryCountdown = NETWORK_RETRIES_PERIOD;
    }
  }

  private void trySwitchToNetwork() {
    if (networkRetryCountdown > 0 && 0 == --networkRetryCountdown) {
      current = network;
    }
  }

  @Override
  public void onNetworkChange(boolean isAvailable) {
    Log.d(TAG, "onNetworkChange: " + isAvailable);
    current = isAvailable ? network : stub;
    networkRetryCountdown = 0;
  }

  private static class StubSink implements UrlsSink {
    @Override
    public void push(String url) {}
  }
}
