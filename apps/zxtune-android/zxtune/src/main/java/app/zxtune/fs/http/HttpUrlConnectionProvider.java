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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;

final class HttpUrlConnectionProvider implements HttpProvider {

  private static final String TAG = HttpUrlConnectionProvider.class.getName();

  /*
    386M http://psf.joshw.info/g/Gekioh Shooting King [Geki Ou - Shienryu] [Shienryu Arcade Hits] (1999-05-20)(Warashi).7z
    Contains psf/psflib files along with nonsupported (xa)

    81M https://psf.joshw.info/f/Final Fantasy IV (1997-03-21)(Square)(Tose)(Square).7z
    Contains 54 psf2 files
  */
  static final int MAX_REMOTE_FILE_SIZE = 100 * 1024 * 1024;

  interface Policy {
    boolean hasConnection();
    void checkConnectionError() throws IOException;
    void checkSizeLimit(int size) throws IOException;
  }

  private final Policy policy;
  private final String userAgent;
  private ByteBuffer htmlBuffer;

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

  @NonNull
  @Override
  public final ByteBuffer getContent(Uri uri) throws IOException {
    final HttpURLConnection connection = connect(uri);
    try {
      final InputStream stream = connection.getInputStream();
      final int size = connection.getContentLength();
      Log.d(TAG, "Fetch %d bytes via %s", size, uri);
      policy.checkSizeLimit(size);
      return getContent(stream, size > 0 ? ByteBuffer.wrap(new byte[size]) : null);
    } finally {
      connection.disconnect();
    }
  }

  @Override
  public void getContent(Uri uri, OutputStream output) throws IOException {
    final HttpURLConnection connection = connect(uri);
    try {
      final byte[] buf = new byte[65536];
      final InputStream stream = connection.getInputStream();
      Log.d(TAG, "Fetch %d bytes stream via %s", connection.getContentLength(), uri);
      for (;;) {
        final int portion = readPartialContent(stream, buf, 0);
        if (portion != 0) {
          output.write(buf, 0, portion);
        } else {
          break;
        }
      }
    } finally {
      connection.disconnect();
    }
  }

  @NonNull
  @Override
  public synchronized String getHtml(Uri uri) throws IOException {
    final HttpURLConnection connection = connect(uri);
    try {
      final InputStream stream = connection.getInputStream();
      htmlBuffer = getContent(stream, htmlBuffer);
      return new String(htmlBuffer.array(), 0, htmlBuffer.limit(), "UTF-8");
    } finally {
      connection.disconnect();
    }
  }

  //! result buffer is not direct so required wrapping
  @NonNull
  private ByteBuffer getContent(InputStream stream, @Nullable ByteBuffer cache) throws IOException {
    byte[] buffer = cache != null ? cache.array() : new byte[1024 * 1024];
    int size = 0;
    for (; ; ) {
      size = readPartialContent(stream, buffer, size);
      if (size == buffer.length) {
        policy.checkSizeLimit(size);
        buffer = reallocate(buffer);
      } else {
        break;
      }
    }
    if (0 == size) {
      throw new IOException("Empty file specified");
    }
    Log.d(TAG, "Got %d bytes", size);
    return ByteBuffer.wrap(buffer, 0, size);
  }

  private static int readPartialContent(InputStream stream, byte[] buffer, int offset) throws IOException {
    final int len = buffer.length;
    while (offset < len) {
      final int chunk = stream.read(buffer, offset, len - offset);
      if (chunk < 0) {
        break;
      }
      offset += chunk;
    }
    return offset;
  }

  private static byte[] reallocate(byte[] buf) {
    final byte[] result = new byte[Math.min(buf.length * 2, MAX_REMOTE_FILE_SIZE + 1)];
    System.arraycopy(buf, 0, result, 0, buf.length);
    return result;
  }
}
