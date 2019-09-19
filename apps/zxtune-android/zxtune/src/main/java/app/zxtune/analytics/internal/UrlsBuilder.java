package app.zxtune.analytics.internal;

import android.net.Uri;

public class UrlsBuilder {

  public static final String DEFAULT_STRING_VALUE = "";
  public static final long DEFAULT_LONG_VALUE = -1;

  private StringBuilder delegate;

  public UrlsBuilder(String type) {
    this.delegate = new StringBuilder(512);
    delegate.append(type);
    delegate.append("?ts=");
    delegate.append(System.currentTimeMillis() / 1000);
  }

  public final void addParam(String key, String value) {
    if (value != null && !value.isEmpty()) {
      addDelimiter();
      delegate.append(key);
      delegate.append('=');
      delegate.append(Uri.encode(value));
    }
  }

  public final void addParam(String key, long value) {
    if (value != DEFAULT_LONG_VALUE) {
      addDelimiter();
      delegate.append(key);
      delegate.append('=');
      delegate.append(value);
    }
  }

  public final void addUri(Uri uri) {
    addParam("uri", cleanupUri(uri));
  }

  public final String getResult() {
    return delegate.toString();
  }

  private void addDelimiter() {
    delegate.append('&');
  }

  private String cleanupUri(Uri uri) {
    final String scheme = uri.getScheme();
    if (scheme == null) {
      return "root";
    } else if ("file".equals(scheme)) {
      return scheme;
    } else {
      return uri.toString();
    }
  }
}
