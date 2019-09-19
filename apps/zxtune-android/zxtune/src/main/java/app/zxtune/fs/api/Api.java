package app.zxtune.fs.api;

import android.content.Context;
import android.util.Base64;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Locale;

import app.zxtune.BuildConfig;
import app.zxtune.auth.Auth;

public class Api {

  private static final String USER_AGENT = String.format(Locale.US, "%s/%d (%s; %s; %s)",
      BuildConfig.APPLICATION_ID,
      BuildConfig.VERSION_CODE,
      BuildConfig.BUILD_TYPE, BuildConfig.VERSION_NAME, BuildConfig.FLAVOR);

  private static final String ENDPOINT = BuildConfig.API_ROOT;
  private static final String REPLY = "";

  private static String authorization;

  public static void initialize(Context ctx) {
    setAuthorization(Auth.getUserIdentifier(ctx), "");
  }

  private static void setAuthorization(String name, String password) {
    final String credentials = name + ":" + password;
    authorization = "Basic " + Base64.encodeToString(credentials.getBytes(), Base64.DEFAULT);
  }

  public static void postEvent(String url) throws IOException {
    final URL fullUrl = new URL(ENDPOINT + "/events/" + url);
    final String reply = doRequest(fullUrl, "POST");
    if (!REPLY.equals(reply)) {
      throw new IOException("Wrong reply: " + reply);
    }
  }

  private static String doRequest(URL fullUrl, String method) throws IOException {
    final HttpURLConnection connection = (HttpURLConnection) fullUrl.openConnection();
    connection.setRequestProperty("User-Agent", USER_AGENT);
    if (authorization != null) {
      connection.setRequestProperty("Authorization", authorization);
    }
    connection.setRequestMethod(method);
    try {
      connection.connect();

      final int code = connection.getResponseCode();
      if (code != HttpURLConnection.HTTP_OK) {
        throw new IOException(connection.getResponseMessage());
      }

      final InputStream input = connection.getInputStream();
      try {
        final URL realUrl = connection.getURL();
        if (!realUrl.equals(fullUrl)) {
          throw new IOException("Unexpected redirect: " + fullUrl + " -> " + realUrl);
        }
        return readReply(input);
      } finally {
        input.close();
      }
    } finally {
      connection.disconnect();
    }
  }

  private static String readReply(InputStream in) throws IOException {
    final byte[] reply = new byte[256];
    final int replySize = in.read(reply);
    return replySize != -1 ? new String(reply, 0, 0, replySize) : "";
  }
}
