/**
 *
 * @file
 *
 * @brief Http access helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;

import app.zxtune.R;

public class HttpProvider {

  private final static String TAG = HttpProvider.class.getName();
  private final static int MAX_REMOTE_FILE_SIZE = 5 * 1024 * 1024;//5Mb

  private Context context;

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
      Log.d(TAG, String.format("Fetch %d bytes via %s", result.getContentLength(), uri));
      return result;
    } catch (IOException e) {
      Log.d(TAG, "Fetch " + uri, e);
      throw e;
    }
  }

  public final ByteBuffer getContent(String uri) throws IOException {
    try {
      final HttpURLConnection connection = connect(uri);
      try {
        final InputStream stream = connection.getInputStream();
        return getContent(stream);
      } finally {
        connection.disconnect();
      }
    } catch (IOException e) {
      checkConnectionError();
      throw e;
    }
  }

  //! result buffer is not direct so required wrapping
  private ByteBuffer getContent(InputStream stream) throws IOException {
    byte[] buffer = new byte[256 * 1024];
    int size = 0;
    for (;;) {
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
    Log.d(TAG, String.format("Got %d bytes", size));
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
      throw new IOException(context.getString(R.string.file_too_big));
    }
  }

  public final void checkConnectionError() throws IOException {
    if (!hasConnection()) {
      throw new IOException(context.getString(R.string.network_inaccessible));
    }
  }

  private boolean hasConnection() {
    final ConnectivityManager mgr = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    final NetworkInfo info = mgr.getActiveNetworkInfo();
    return info != null && info.isConnected();
  }
}
