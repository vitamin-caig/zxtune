package app.zxtune.analytics.internal;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.LocaleList;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.util.DisplayMetrics;

import java.io.IOException;
import java.util.concurrent.LinkedBlockingQueue;

import app.zxtune.Log;
import app.zxtune.analytics.Analytics;

public final class Provider extends ContentProvider {

  private static final String TAG = Provider.class.getName();

  static final String METHOD_PUSH = "push";

  static final Uri URI = new Uri.Builder()
                             .scheme(ContentResolver.SCHEME_CONTENT)
                             .authority("app.zxtune.analytics.internal")
                             .build();

  private final LinkedBlockingQueue<String> queue = new LinkedBlockingQueue<>(1024);
  private UrlsSink delegate;

  @Override
  public boolean onCreate() {
    final Context ctx = getContext().getApplicationContext();
    Analytics.initialize(ctx);
    delegate = new Dispatcher(ctx);
    final Thread thread = new Thread("IASender") {
      @Override
      public void run() {
        try {
          doSending(queue, delegate);
        } catch (Exception e) {
          Log.w(TAG, e, "Stopped IASender thread");
        }
      }
    };
    thread.setDaemon(true);
    thread.start();
    sendSystemInfoEvent();
    sendSystemConfigurationEvent(ctx.getResources());
    return true;
  }

  private void sendSystemInfoEvent() {
    final UrlsBuilder builder = new UrlsBuilder("system/info");
    builder.addParam("product", Build.PRODUCT);
    builder.addParam("device", Build.DEVICE);
    if (Build.VERSION.SDK_INT >= 21) {
      builder.addParam("cpu_abi", Build.SUPPORTED_ABIS[0]);
      for (int idx = 1; idx < Build.SUPPORTED_ABIS.length; ++idx) {
        builder.addParam("cpu_abi" + (idx + 1), Build.SUPPORTED_ABIS[idx]);
      }
    } else {
      builder.addParam("cpu_abi", Build.CPU_ABI);
      builder.addParam("cpu_abi2", Build.CPU_ABI2);
    }
    builder.addParam("manufacturer", Build.MANUFACTURER);
    builder.addParam("brand", Build.BRAND);
    builder.addParam("model", Build.MODEL);

    builder.addParam("android", Build.VERSION.RELEASE);
    builder.addParam("sdk", Build.VERSION.SDK_INT);

    doPush(builder.getResult());
  }

  private void sendSystemConfigurationEvent(Resources res) {
    final UrlsBuilder builder = new UrlsBuilder("system/config");
    final Configuration cfg = res.getConfiguration();
    final DisplayMetrics metrics = res.getDisplayMetrics();
    builder.addParam("layout", cfg.screenLayout);
    if (Build.VERSION.SDK_INT >= 24) {
      final LocaleList locales = cfg.getLocales();
      builder.addParam("locale", locales.get(0).toString());
      for (int idx = 1, lim = locales.size(); idx < lim; ++idx) {
        builder.addParam("locale" + (idx + 1), locales.get(idx).toString());
      }
    } else {
      builder.addParam("locale", cfg.locale.toString());
    }
    builder.addParam("density", metrics.densityDpi);

    doPush(builder.getResult());
  }

  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
    return null;
  }

  @Nullable
  @Override
  public String getType(@NonNull Uri uri) {
    return null;
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
    return null;
  }

  @Override
  public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
  }

  @Override
  public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
  }

  @Override
  public Bundle call(@NonNull String method, @NonNull String arg, @Nullable Bundle extras) {
    if (METHOD_PUSH.equals(method)) {
      doPush(arg);
    }
    return null;
  }

  private void doPush(String url) {
    queue.offer(url);
  }

  private static void doSending(LinkedBlockingQueue<String> queue, UrlsSink output) throws InterruptedException, IOException {
    for (; ; ) {
      final String url = queue.take();
      output.push(url);
    }
  }
}
