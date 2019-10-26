package app.zxtune.analytics.internal;

import android.content.Context;

import java.io.IOException;

import app.zxtune.fs.api.Api;

final class NetworkSink implements UrlsSink {

  NetworkSink(Context ctx) {
    Api.initialize(ctx);
  }

  @Override
  public void push(String url) throws IOException {
    Api.postEvent(url);
  }
}
