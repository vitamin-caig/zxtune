/**
 * @file
 * @brief Http access helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;

import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.R;

public class HttpProvider {

  private static final String TAG = HttpProvider.class.getName();

  /*
    386M http://psf.joshw.info/g/Gekioh Shooting King [Geki Ou - Shienryu] [Shienryu Arcade Hits] (1999-05-20)(Warashi).7z
    Contains psf/psflib files along with nonsupported (xa)
    
    81M https://psf.joshw.info/f/Final Fantasy IV (1997-03-21)(Square)(Tose)(Square).7z
    Contains 54 psf2 files
  */
  private static final int MAX_REMOTE_FILE_SIZE = 100 * 1024 * 1024;

  private final Context context;
  private ByteBuffer htmlBuffer;

  public HttpProvider(Context context) {
    this.context = context;
  }

  public final HttpURLConnection connect(Uri uri) throws IOException {
    try {
      final URL url = new URL(uri.toString());
      return (HttpURLConnection) url.openConnection();
    } catch (IOException e) {
      Log.d(TAG, "Fetch %s: %s", uri, e.toString());
      throw e;
    }
  }

  @NonNull
  public final ByteBuffer getContent(Uri uri) throws IOException {
    try {
      final HttpURLConnection connection = connect(uri);
      try {
        final InputStream stream = connection.getInputStream();
        final int size = connection.getContentLength();
        Log.d(TAG, "Fetch %d bytes via %s", size, uri);
        CheckSizeLimit(size);
        return getContent(stream, size > 0 ? ByteBuffer.wrap(new byte[size]) : null);
      } finally {
        connection.disconnect();
      }
    } catch (IOException e) {
      checkConnectionError();
      throw e;
    }
  }

  public final void getContent(Uri uri, OutputStream output) throws IOException {
    try {
      final HttpURLConnection connection = connect(uri);
      try {
        final byte[] buf = new byte[65536];
        final InputStream stream = connection.getInputStream();
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
    } catch (IOException e) {
      checkConnectionError();
      throw e;
    }
  }

  @NonNull
  public final synchronized String getHtml(Uri uri) throws IOException {
    try {
      final HttpURLConnection connection = connect(uri);
      try {
        final InputStream stream = connection.getInputStream();
        htmlBuffer = getContent(stream, htmlBuffer);
        return new String(htmlBuffer.array(), 0, htmlBuffer.limit(), "UTF-8");
      } finally {
        connection.disconnect();
      }
    } catch (IOException e) {
      checkConnectionError();
      throw e;
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
        CheckSizeLimit(size);
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

  private void CheckSizeLimit(int size) throws IOException {
    if (size > MAX_REMOTE_FILE_SIZE) {
      Analytics.sendTooBigFileEvent(size);
      throw new IOException(context.getString(R.string.file_too_big));
    }
  }

  public final void checkConnectionError() throws IOException {
    if (!hasConnection()) {
      throw new IOException(context.getString(R.string.network_inaccessible));
    }
  }

  public final boolean hasConnection() {
    final ConnectivityManager mgr = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    final NetworkInfo info = mgr != null ? mgr.getActiveNetworkInfo() : null;
    return info != null && info.isConnected();
  }
}
