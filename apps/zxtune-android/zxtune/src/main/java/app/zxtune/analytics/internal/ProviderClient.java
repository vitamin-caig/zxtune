package app.zxtune.analytics.internal;

import android.content.ContentResolver;
import android.content.Context;

import app.zxtune.Log;
import app.zxtune.utils.AsyncWorker;

final class ProviderClient implements UrlsSink {

  private static final String TAG = ProviderClient.class.getName();

  private final AsyncWorker worker = new AsyncWorker("IASender");
  private final ContentResolver resolver;

  ProviderClient(Context ctx) {
    resolver = ctx.getContentResolver();
  }

  @Override
  public void push(String msg) {
    worker.execute(() -> doPush(msg));
  }

  private void doPush(String msg) {
    try {
      resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null);
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to push url");
    }
  }
}
