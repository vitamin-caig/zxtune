package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import app.zxtune.Analytics;
import app.zxtune.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;

public final class MultisourceHttpProvider {

  private static final String TAG = MultisourceHttpProvider.class.getName();

  private static final long QUARANTINE_PERIOD_MS = 60 * 60 * 1000;

  private HttpProvider delegate;
  private HashMap<String, Long> hostDisabledTill = new HashMap<>();

  public MultisourceHttpProvider(HttpProvider delegate) {
    this.delegate = delegate;
  }

  @NonNull
  public final HttpObject getObject(Uri[] uris) throws IOException {
    //check out only primary
    return delegate.getObject(uris[0]);
  }

  @NonNull
  public final ByteBuffer getContent(Uri[] uris) throws IOException {
    final long now = System.currentTimeMillis();
    for (int idx = 0; ; ++idx) {
      final Uri uri = uris[idx];
      final boolean isLast = idx == uris.length - 1;
      if (!isLast && isDisabled(uri, now)) {
        continue;
      }
      try {
        return delegate.getContent(uri);
      } catch (IOException ex) {
        if (isLast || !delegate.hasConnection()) {
          throw ex;
        } else {
          disable(uri, now);
        }
      }
    }
  }

  public final void getContent(Uri[] uris, OutputStream target) throws IOException {
    final long now = System.currentTimeMillis();
    for (int idx = 0; ; ++idx) {
      final Uri uri = uris[idx];
      final boolean isLast = idx == uris.length - 1;
      if (!isLast && isDisabled(uri, now)) {
        continue;
      }
      try {
        delegate.getContent(uri, target);
        return;
      } catch (IOException ex) {
        if (isLast || !delegate.hasConnection()) {
          throw ex;
        } else {
          disable(uri, now);
        }
      }
    }
  }

  private boolean isDisabled(Uri uri, long now) {
    final String host = uri.getHost();
    return hostDisabledTill.containsKey(host) && hostDisabledTill.get(host) > now;
  }

  private void disable(Uri uri, long now) {
    final String host = uri.getHost();
    Log.d(TAG, "Temporarily disable requests to %s", host);
    Analytics.sendHostUnavailableEvent(host);
    hostDisabledTill.put(host, now + QUARANTINE_PERIOD_MS);
  }
}
