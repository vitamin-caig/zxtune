package app.zxtune.analytics.internal;

import android.content.Context;

public final class Factory {

  public static UrlsSink createClientEndpoint(Context ctx) {
    return new ProviderClient(ctx);
  }
}
