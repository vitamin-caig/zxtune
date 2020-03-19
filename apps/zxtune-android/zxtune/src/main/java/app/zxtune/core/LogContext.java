package app.zxtune.core;

import android.net.Uri;

import java.util.Locale;

import app.zxtune.analytics.Analytics;

class LogContext {
  private final String prefix;

  public LogContext(Uri uri, String subpath, int size) {
    this.prefix = String.format(Locale.US, "%d: ", uri.hashCode() ^ subpath.hashCode());
    action(String.format(Locale.US, "file=%s subpath=%s size=%d",
        "file".equals(uri.getScheme()) ? uri.getLastPathSegment() : uri.toString(),
        subpath, size));
  }

  public final void action(String action) {
    Analytics.logMessage(prefix + action);
  }
}
