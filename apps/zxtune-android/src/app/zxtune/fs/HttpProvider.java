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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;

import app.zxtune.R;

public class HttpProvider {

  private final static String TAG = HttpProvider.class.getName();

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

  private static ByteBuffer getContent(InputStream stream) throws IOException {
    final ByteArrayOutputStream buffer = new ByteArrayOutputStream();
    final byte[] part = new byte[16384];
    for (;;) {
      final int read = readPartialContent(stream, part);
      buffer.write(part, 0, read);
      if (read != part.length) {
        break;
      }
    }
    if (0 == buffer.size()) {
      throw new IOException("Empty file specified");
    }
    Log.d(TAG, String.format("Got %d bytes", buffer.size()));
    return ByteBuffer.wrap(buffer.toByteArray());
  }

  private static int readPartialContent(InputStream stream, byte[] buffer) throws IOException {
    final int len = buffer.length;
    int received = 0;
    while (received < len) {
      final int chunk = stream.read(buffer, received, len - received);
      if (chunk < 0) {
        break;
      }
      received += chunk;
    }
    return received;
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
