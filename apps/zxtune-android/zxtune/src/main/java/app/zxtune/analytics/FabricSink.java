package app.zxtune.analytics;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.LocaleList;

import androidx.collection.SparseArrayCompat;

import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.ndk.CrashlyticsNdk;

import app.zxtune.BuildConfig;
import app.zxtune.core.Player;
import app.zxtune.playback.PlayableItem;
import io.fabric.sdk.android.Fabric;

final class FabricSink implements Sink {

  //private static final String TAG = FabricSink.class.getName();

  static boolean isEnabled() {
    return BuildConfig.BUILD_TYPE.equals("release");
  }

  FabricSink(Context ctx) {
    Fabric.with(ctx, new Crashlytics(), new CrashlyticsNdk());
    setTags(ctx);
  }

  private void setTags(Context ctx) {
    try {
      Crashlytics.setString("installer", getInstaller(ctx));
      Crashlytics.setString("arch", Build.CPU_ABI);
      Crashlytics.setString("locale", getLocale(ctx));
    } catch (Throwable e) {
      logException(e);
    }
  }

  private static String getInstaller(Context ctx) {
    final PackageManager pm = ctx.getPackageManager();
    final String res = pm.getInstallerPackageName(ctx.getPackageName());
    return res != null ? res : "unknown";
  }

  private static String getLocale(Context ctx) {
    final Configuration cfg = ctx.getResources().getConfiguration();
    if (Build.VERSION.SDK_INT >= 24) {
      final LocaleList locales = cfg.getLocales();
      return locales.get(0).toString();
    } else {
      return cfg.locale.toString();
    }
  }

  @Override
  public void logException(Throwable e) {
    Crashlytics.logException(e);
  }

  @Override
  public void sendTrace(String tag, SparseArrayCompat<String> points) {}

  @Override
  public void sendPlayEvent(PlayableItem item, Player player) {}

  @Override
  public void sendBrowserEvent(Uri path, int action) {}

  @Override
  public void sendSocialEvent(Uri path, String app, int action) {}

  @Override
  public void sendUiEvent(int action) {}

  @Override
  public void sendPlaylistEvent(int action, int param) {}

  @Override
  public void sendVfsEvent(String id, String scope, int action) {}

  @Override
  public void sendNoTracksFoundEvent(Uri uri) {}
}
