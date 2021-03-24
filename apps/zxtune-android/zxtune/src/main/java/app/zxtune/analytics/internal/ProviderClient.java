package app.zxtune.analytics.internal;

import android.content.ContentResolver;
import android.content.Context;

import app.zxtune.utils.AsyncWorker;

final class ProviderClient implements UrlsSink {

  private final AsyncWorker worker = new AsyncWorker("IASender");
  private final ContentResolver resolver;

  ProviderClient(Context ctx) {
    resolver = ctx.getContentResolver();
  }

  @Override
  public void push(String msg) {
    worker.execute(() -> resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null));
  }
}
