package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;
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
  public final ByteBuffer getContent(Uri[] uris) throws IOException {
    final long now = System.currentTimeMillis();
    for (int idx = 0; ; ++idx) {
      final Uri uri = uris[idx];
      final String host = uri.getHost();
      final boolean isLast = idx == uris.length;
      if (!isLast && isDisabled(host, now)) {
        continue;
      }
      try {
        return delegate.getContent(uri);
      } catch (IOException ex) {
        if (isLast) {
          throw ex;
        } else {
          disable(host, now);
        }
      }
    }
  }

  public final void getContent(Uri[] uris, OutputStream target) throws IOException {
    final long now = System.currentTimeMillis();
    for (int idx = 0; ; ++idx) {
      final Uri uri = uris[idx];
      final String host = uri.getHost();
      if (idx != uris.length - 1 && isDisabled(host, now)) {
        continue;
      }
      try {
        delegate.getContent(uri, target);
        return;
      } catch (IOException ex) {
        disable(host, now);
      }
    }
  }

  private boolean isDisabled(String host, long now) {
    return hostDisabledTill.containsKey(host) && hostDisabledTill.get(host) > now;
  }

  private void disable(String host, long now) {
    Log.d(TAG, "Temporarily disable requests to %s", host);
    hostDisabledTill.put(host, now + QUARANTINE_PERIOD_MS);
  }
}
