package app.zxtune.fs.http;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import java.io.IOException;

import app.zxtune.Analytics;
import app.zxtune.BuildConfig;
import app.zxtune.R;

public final class HttpProviderFactory {

  private static class PolicyImpl implements HttpUrlConnectionProvider.Policy {

    private final Context context;
    private final ConnectivityManager manager;

    PolicyImpl(Context ctx) {
      this.context = ctx;
      this.manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
    }

    @Override
    public boolean hasConnection() {
      final NetworkInfo info = manager != null ? manager.getActiveNetworkInfo() : null;
      return info != null && info.isConnected();
    }

    @Override
    public void checkConnectionError() throws IOException {
      if (!hasConnection()) {
        throw new IOException(context.getString(R.string.network_inaccessible));
      }
    }

    @Override
    public void checkSizeLimit(int size) throws IOException {
      if (size > HttpUrlConnectionProvider.MAX_REMOTE_FILE_SIZE) {
        Analytics.sendTooBigFileEvent(size);
        throw new IOException(context.getString(R.string.file_too_big));
      }
    }
  }

  public static HttpProvider createProvider(Context ctx) {
    final PolicyImpl policy = new PolicyImpl(ctx);
    final String userAgent = String.format("%s/%d (%s; %s; %s)", BuildConfig.APPLICATION_ID, BuildConfig.VERSION_CODE,
        BuildConfig.BUILD_TYPE, BuildConfig.VERSION_NAME, BuildConfig.FLAVOR);
    return new HttpUrlConnectionProvider(policy, userAgent);
  }
}
