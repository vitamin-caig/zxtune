/**
 * @file
 * @brief Http access helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.http;

import android.net.Uri;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.zxtune.TimeStamp;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.TimeUnit;

final class HttpUrlConnectionProvider implements HttpProvider {

  private static final String TAG = HttpUrlConnectionProvider.class.getName();

  interface Policy {
    boolean hasConnection();

    void checkConnectionError() throws IOException;
  }

  private final Policy policy;
  private final String userAgent;

  HttpUrlConnectionProvider(Policy policy, String userAgent) {
    this.policy = policy;
    this.userAgent = userAgent;
  }

  @Override
  public boolean hasConnection() {
    return policy.hasConnection();
  }

  @NonNull
  @Override
  public HttpObject getObject(Uri uri) throws IOException {
    try {
      return new SimpleHttpObject(uri);
    } catch (IOException e) {
      policy.checkConnectionError();
      throw e;
    }
  }

  private class SimpleHttpObject implements HttpObject {

    private final Long contentLength;
    private final TimeStamp lastModified;
    private final Uri uri;
    private final boolean acceptRanges;

    SimpleHttpObject(Uri uri) throws IOException {
      final HttpURLConnection connection = createConnection(uri);
      connection.setRequestProperty("Accept-Encoding", "identity");
      connection.setRequestMethod("HEAD");
      connection.setDoInput(false);
      connection.connect();

      final int code = connection.getResponseCode();
      if (code != HttpURLConnection.HTTP_OK) {
        throw new IOException(connection.getResponseMessage());
      }
      this.contentLength = HttpUrlConnectionProvider.getContentLength(connection);
      this.lastModified = HttpUrlConnectionProvider.getLastModified(connection);
      //may be different from original uri
      this.uri = Uri.parse(connection.getURL().toString());
      this.acceptRanges = HttpUrlConnectionProvider.hasAcceptRanges(connection.getHeaderFields());

      connection.disconnect();
    }

    @NonNull
    @Override
    public Uri getUri() {
      return uri;
    }

    @Nullable
    @Override
    public Long getContentLength() {
      return contentLength;
    }

    @Nullable
    @Override
    public TimeStamp getLastModified() {
      return lastModified;
    }

    @NonNull
    @Override
    public InputStream getInput() throws IOException {
      if (contentLength != null) {
        return new FixedSizeInputStream(uri, contentLength, acceptRanges);
      } else {
        return createStream(uri, 0);
      }
    }
  }

  private static Long getContentLength(HttpURLConnection connection) {
    final long size = Build.VERSION.SDK_INT >= 24 ? connection.getContentLengthLong() : connection.getContentLength();
    return size >= 0 ? size : null;
  }

  private static TimeStamp getLastModified(HttpURLConnection connection) {
    final long stamp = connection.getLastModified();
    return stamp > 0 ? TimeStamp.createFrom(stamp, TimeUnit.MILLISECONDS) : null;
  }

  private static boolean hasAcceptRanges(Map<String, List<String>> fields) {
    for (String name : fields.keySet()) {
      if (name != null && 0 == name.compareToIgnoreCase("accept-ranges")) {
        for (String value : fields.get(name)) {
          if (value != null && 0 == value.compareToIgnoreCase("bytes")) {
            return true;
          }
        }
      }
    }
    return false;
  }

  @NonNull
  @Override
  public InputStream getInputStream(Uri uri) throws IOException {
    return createStream(uri, 0);
  }

  private HttpURLConnection createConnection(Uri uri) throws IOException {
    final URL url = new URL(uri.toString());
    final HttpURLConnection result = (HttpURLConnection) url.openConnection();
    result.setRequestProperty("User-Agent", userAgent);
    return result;
  }

  private HttpInputStream createStream(Uri uri, long offset) throws IOException {
    try {
      final HttpURLConnection connection = createConnection(uri);
      if (offset != 0) {
        connection.setRequestProperty("Range", "bytes=" + offset + "-");
      }
      return new HttpInputStream(connection);
    } catch (IOException e) {
      policy.checkConnectionError();
      throw e;
    }
  }

  private class FixedSizeInputStream extends InputStream {

    private final Uri uri;
    private final long total;
    private final boolean acceptRanges;
    private long done;
    private HttpInputStream delegate;

    FixedSizeInputStream(Uri uri, long totalSize, boolean acceptRanges) throws IOException {
      this.uri = uri;
      this.total = totalSize;
      this.acceptRanges = acceptRanges;
      this.done = 0;
      this.delegate = createStream();
    }

    @Override
    public void close() {
      if (delegate != null) {
        delegate.close();
        delegate = null;
      }
    }

    @Override
    public int available() throws IOException {
      return delegate.available();
    }

    @Override
    public int read() throws IOException {
      while (done < total) {
        final int res = delegate.read();
        if (res == -1) {
          reopenDelegate();
        } else {
          ++done;
          return res;
        }
      }
      return -1;
    }

    @Override
    public int read(@NonNull byte[] b, int off, int len) throws IOException {
      if (done == total) {
        return -1;
      }
      final long doneBefore = done;
      while (len > 0 && done < total) {
        final int res = delegate.read(b, off, len);
        if (res < 0) {
          reopenDelegate();
        } else {
          done += res;
          off += res;
          len -= res;
        }
      }
      return (int) (done - doneBefore);
    }

    @Override
    public long skip(long n) throws IOException {
      final long doneBefore = done;
      while (n > 0 && done < total) {
        final long res = delegate.skip(n);
        if (res == 0) {
          reopenDelegate();
        } else {
          done += res;
          n -= res;
        }
      }
      return done - doneBefore;
    }

    private void reopenDelegate() throws IOException {
      final HttpInputStream stream = createStream();
      close();
      delegate = stream;
    }

    private HttpInputStream createStream() throws IOException {
      if (done != 0 && !acceptRanges) {
        throw new IOException(String.format(Locale.US, "Got %d out of %d bytes while reading from %s", done, total, uri));
      }
      return HttpUrlConnectionProvider.this.createStream(uri, done);
    }
  }
}
