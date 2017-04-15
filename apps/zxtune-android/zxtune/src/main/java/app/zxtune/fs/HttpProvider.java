/**
 * @file
 * @brief Http access helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.io.InputStream;
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
  private static ByteBuffer htmlBuffer;

  public HttpProvider(Context context) {
    this.context = context;
    // HTTP connection reuse which was buggy pre-froyo
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.FROYO) {
      System.setProperty("http.keepAlive", "false");
    }
  }


  public final HttpURLConnection connect(String uri) throws IOException {
    try {
      final URL url = new URL(uri);
      final HttpURLConnection result = (HttpURLConnection) url.openConnection();
      CheckSizeLimit(result.getContentLength());
      Log.d(TAG, "Fetch %d bytes via %s", result.getContentLength(), uri);
      return result;
    } catch (IOException e) {
      Log.d(TAG, "Fetch %s: %s", uri, e.toString());
      throw e;
    }
  }


  public final ByteBuffer getContent(String uri) throws IOException {
    try {
      final HttpURLConnection connection = connect(uri);
      try {
        final InputStream stream = connection.getInputStream();
        return getContent(stream, null);
      } finally {
        connection.disconnect();
      }
    } catch (IOException e) {
      checkConnectionError();
      throw e;
    }
  }


  public final synchronized String getHtml(String uri) throws IOException {
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

  private ByteBuffer getContent(InputStream stream, @Nullable ByteBuffer cache) throws IOException {
    byte[] buffer = cache != null ? cache.array() : new byte[256 * 1024];
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
    final byte[] result = new byte[buf.length * 2];
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
    final NetworkInfo info = mgr.getActiveNetworkInfo();
    return info != null && info.isConnected();
  }
}
