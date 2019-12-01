package app.zxtune.net;

import android.os.Build;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Locale;

import app.zxtune.BuildConfig;

public final class Http {
  private static final String USER_AGENT = String.format(Locale.US, "%s/%d (%s; %s; %s)",
      BuildConfig.APPLICATION_ID,
      BuildConfig.VERSION_CODE,
      BuildConfig.BUILD_TYPE, Build.CPU_ABI, BuildConfig.FLAVOR);

  public static HttpURLConnection createConnection(String uri) throws IOException {
    final URL url = new URL(uri);
    final HttpURLConnection result = (HttpURLConnection) url.openConnection();
    result.setRequestProperty("User-Agent", USER_AGENT);
    return result;
  }
}
