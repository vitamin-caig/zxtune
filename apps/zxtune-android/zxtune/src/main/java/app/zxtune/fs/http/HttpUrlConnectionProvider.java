/**
 * @file
 * @brief Http access helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.http;

import android.net.Uri;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import app.zxtune.TimeStamp;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
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
    return new SimpleHttpObject(uri);
  }

  private class SimpleHttpObject implements HttpObject {

    private final Long contentLength;
    private final TimeStamp lastModified;
    private final Uri uri;

    SimpleHttpObject(Uri uri) throws IOException {
      final HttpURLConnection connection = connect(uri);
      connection.addRequestProperty("Accept-Encoding", "identity");
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
      final HttpURLConnection connection = connect(uri);
      return new HttpInputStream(connection);
    }
  }

  private static Long getContentLength(HttpURLConnection connection) {
    final long size = Build.VERSION.SDK_INT >= 24 ? connection.getContentLengthLong() : connection.getContentLength();
    return size >= 0 ? Long.valueOf(size) : null;
  }

  private static TimeStamp getLastModified(HttpURLConnection connection) {
    final long stamp = connection.getLastModified();
    return stamp > 0 ? TimeStamp.createFrom(stamp, TimeUnit.MILLISECONDS) : null;
  }

  @NonNull
  @Override
  public InputStream getInputStream(Uri uri) throws IOException {
    final HttpURLConnection connection = connect(uri);
    return new HttpInputStream(connection);
  }

  private HttpURLConnection connect(Uri uri) throws IOException {
    try {
      final URL url = new URL(uri.toString());
      final HttpURLConnection result = (HttpURLConnection) url.openConnection();
      result.setRequestProperty("User-Agent", userAgent);
      return result;
    } catch (IOException e) {
      policy.checkConnectionError();
      throw e;
    }
  }
}
