package app.zxtune.analytics.internal;

import java.io.IOException;

public interface UrlsSink {
  void push(String url) throws IOException;
}
