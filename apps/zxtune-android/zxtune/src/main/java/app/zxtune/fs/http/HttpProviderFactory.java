package app.zxtune.fs.http;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import androidx.annotation.Nullable;

import java.io.IOException;

import app.zxtune.R;

public final class HttpProviderFactory {

  private static class PolicyImpl implements HttpUrlConnectionProvider.Policy {

    private final Context context;
    @Nullable
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
  }

  public static HttpProvider createProvider(Context ctx) {
    final PolicyImpl policy = new PolicyImpl(ctx);
    return new HttpUrlConnectionProvider(policy);
  }

  public static HttpProvider createTestProvider() {
    return new HttpUrlConnectionProvider(new HttpUrlConnectionProvider.Policy() {
      @Override
      public boolean hasConnection() {
        return true;
      }

      @Override
      public void checkConnectionError() {}
    });
  }
}
