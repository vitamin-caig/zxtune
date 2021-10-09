package app.zxtune.fs.http;

import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

import app.zxtune.Log;

public final class MultisourceHttpProvider {

  interface TimeSource {
    long nowMillis();
  }

  private static final String TAG = MultisourceHttpProvider.class.getName();

  private static final long MIN_QUARANTINE_PERIOD = 1000;
  private static final long MAX_QUARANTINE_PERIOD = 60 * 60 * 1000;

  private final HttpProvider delegate;
  private final TimeSource time;
  private final HashMap<String, HostStatistics> hostStat = new HashMap<>();

  public MultisourceHttpProvider(HttpProvider delegate) {
    this(delegate, System::currentTimeMillis);
  }

  @VisibleForTesting
  MultisourceHttpProvider(HttpProvider delegate, TimeSource time) {
    this.delegate = delegate;
    this.time = time;
  }

  public final boolean hasConnection() {
    return delegate.hasConnection();
  }

  public final HttpObject getObject(Uri[] uris) throws IOException {
    return getSingleObject(uris, delegate::getObject);
  }

  public final InputStream getInputStream(Uri uri) throws IOException {
    return delegate.getInputStream(uri);
  }

  public final InputStream getInputStream(Uri[] uris) throws IOException {
    return getSingleObject(uris, delegate::getInputStream);
  }

  private interface Getter<T> {
    T get(Uri uri) throws IOException;
  }

  private <T> T getSingleObject(Uri[] uris, Getter<T> getter) throws IOException {
    final long now = time.nowMillis();
    for (int idx = 0; ; ++idx) {
      final Uri uri = uris[idx];
      final String host = uri.getHost();
      final HostStatistics stat = getStat(host);
      final boolean isLast = idx == uris.length - 1;
      if (!isLast && stat.isDisabledFor(now)) {
        continue;
      }
      try {
        final T result = getter.get(uri);
        stat.onSuccess(now);
        return result;
      } catch (IOException ex) {
        if (isLast || !delegate.hasConnection()) {
          throw ex;
        } else {
          stat.onError(now, new IOException(ex));
        }
      }
    }
  }

  @VisibleForTesting
  boolean isHostDisabledFor(String host, long time) {
    final HostStatistics stat = hostStat.get(host);
    return stat != null && stat.isDisabledFor(time);
  }

  private synchronized HostStatistics getStat(@Nullable String host) {
    final HostStatistics existing = hostStat.get(host);
    if (existing != null) {
      return existing;
    } else {
      final HostStatistics newOne = new HostStatistics(host);
      hostStat.put(host, newOne);
      return newOne;
    }
  }

  private static class HostStatistics {
    @Nullable
    private final String host;
    private int successes;
    private long lastSuccess;
    private int errors;
    private long lastError;
    private long disabledTill;

    HostStatistics(@Nullable String host) {
      this.host = host;
    }

    synchronized boolean isDisabledFor(long now) {
      return disabledTill > now;
    }

    synchronized void onSuccess(long now) {
      ++successes;
      lastSuccess = now;
    }

    void onError(long now, IOException ex) {
      if (disableFor(now)) {
        Log.w(TAG, ex, "Temporarily disable requests to '%s' (%d/%d)", host, successes, errors);
      } else {
        Log.w(TAG, ex, "Give a change after error for host '%s' (%d/%d)", host, successes, errors);
      }
    }

    private synchronized boolean disableFor(long now) {
      final long quarantine = getQuarantine();
      ++errors;
      lastError = now;
      disabledTill = now + quarantine;
      return quarantine != 0;
    }

    private long getQuarantine() {
      if (lastSuccess >= lastError) {
        // forgive first error in chain
        return 0;
      } else if (successes >= errors) {
        return MIN_QUARANTINE_PERIOD;
      } else {
        return Math.min(MAX_QUARANTINE_PERIOD, MIN_QUARANTINE_PERIOD * (errors - successes));
      }
    }
  }
}
