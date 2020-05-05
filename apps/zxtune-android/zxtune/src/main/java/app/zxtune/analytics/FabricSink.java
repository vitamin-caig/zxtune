package app.zxtune.analytics;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.LocaleList;

import app.zxtune.BuildConfig;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Player;
import app.zxtune.playback.PlayableItem;
import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.answers.CustomEvent;
import com.crashlytics.android.ndk.CrashlyticsNdk;

import app.zxtune.playlist.ProviderClient;
import io.fabric.sdk.android.Fabric;

import java.util.List;
import java.util.concurrent.TimeUnit;

final class FabricSink implements Sink {

  private static final String TAG = FabricSink.class.getName();

  static boolean isEnabled() {
    return BuildConfig.BUILD_TYPE.equals("release");
  }

  FabricSink(Context ctx) {
    Fabric.with(ctx, new Crashlytics(), new CrashlyticsNdk(), new Answers());
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
  public void logMessage(String msg) {
    Crashlytics.log(msg);
  }

  @Override
  public void sendPlayEvent(PlayableItem item, Player player) {
    try {
      final Identifier id = item.getDataId();
      final Uri location = id.getFullLocation();
      final Module module = item.getModule();
      final String type = module.getProperty(ModuleAttributes.TYPE, "Unknown");
      final String program = module.getProperty(ModuleAttributes.PROGRAM, "Unknown");
      final String container = module.getProperty(ModuleAttributes.CONTAINER, "None");
      final TimeStamp duration = item.getDuration();
      final boolean fromBrowser = item.getId().equals(location);

      final CustomEvent event = new CustomEvent("Play");
      fillSource(event, location);
      event.putCustomAttribute("Type", type)
          .putCustomAttribute("TypeDetailed", type + "/" + program)
          .putCustomAttribute("Container", container)
          .putCustomAttribute("Duration", duration.convertTo(TimeUnit.SECONDS))
          .putCustomAttribute("Library", fromBrowser ? "Browser" : "Playlist")
      ;
      send(event);

      sendPerformanceEvent(player.getPerformance(), type);
    } catch (Exception e) {
      Log.w(TAG, e, "sendPlayEvent");
    }
  }

  private static void sendPerformanceEvent(int perf, String type) {
    if (perf != 0) {
      final CustomEvent event = new CustomEvent("Performance");
      event.putCustomAttribute(type + " playback,%", perf);
      send(event);
    }
  }

  @Override
  public void sendBrowserEvent(Uri path, int action) {

    final Identifier id = new Identifier(path);
    final int archiveDepth = id.getSubpathComponents().length;
    final int depth = path.getPathSegments().size() + archiveDepth;
    final boolean isArchive = archiveDepth != 0;

    final CustomEvent event = new CustomEvent(serializeBrowserAction(action));
    fillSource(event, path);
    event.putCustomAttribute("Depth", depth)
        .putCustomAttribute("Type", isArchive ? "Archive" : "Folder")
    ;
    send(event);
  }

  private static String serializeBrowserAction(@Analytics.BrowserAction int action) {
    switch (action) {
      case Analytics.BROWSER_ACTION_BROWSE:
      case Analytics.BROWSER_ACTION_BROWSE_PARENT:
        return "Browse";
      case Analytics.BROWSER_ACTION_SEARCH:
        return "Search";
      default:
        return "";
    }
  }

  @Override
  public void sendSocialEvent(Uri path, String app, int action) {
    final CustomEvent event = new CustomEvent("Social");
    fillSource(event, path);
    final String method = serializeSocialAction(action);
    event.putCustomAttribute("Method", method)
        .putCustomAttribute("Application", app)
        .putCustomAttribute("MethodDetailed", method + "/" + app)
    ;
    send(event);
  }

  private static String serializeSocialAction(@Analytics.SocialAction int action) {
    switch (action) {
      case Analytics.SOCIAL_ACTION_RINGTONE:
        return "Ringtone";
      case Analytics.SOCIAL_ACTION_SHARE:
        return "Share";
      case Analytics.SOCIAL_ACTION_SEND:
        return "Send";
      default:
        return "";
    }
  }

  private static void fillSource(CustomEvent event, Uri path) {
    String scheme = path.getScheme();
    if (scheme == null) {
      scheme = "root";
    }
    final List<String> segments = path.getPathSegments();
    final String pathStart = segments.isEmpty() ? "" : segments.get(0);
    event.putCustomAttribute("Source", scheme);
    event.putCustomAttribute("SourceDetailed", scheme + "/" + pathStart);
  }

  @Override
  public void sendUiEvent(int action) {
    final String actionStr = serializeUiAction(action);
    if (!actionStr.isEmpty()) {
      final CustomEvent event = new CustomEvent("UI");
      event.putCustomAttribute("Action", actionStr);
      send(event);
    }
  }

  private static String serializeUiAction(@Analytics.UiAction int action) {
    switch (action) {
      case Analytics.UI_ACTION_PREFERENCES:
        return "Preferences";
      case Analytics.UI_ACTION_RATE:
        return "Rate";
      case Analytics.UI_ACTION_ABOUT:
        return "About";
      case Analytics.UI_ACTION_QUIT:
        return "Quit";
      default:
        return "";
    }
  }

  @Override
  public void sendPlaylistEvent(int action, int param) {
    final CustomEvent event = new CustomEvent("Playlist");
    final String act = serializePlaylistAction(action);
    event.putCustomAttribute("Action", act);
    if (action == Analytics.PLAYLIST_ACTION_ADD) {
      event.putCustomAttribute(act, param);
    } else if (action == Analytics.PLAYLIST_ACTION_SAVE) {
      //compatibility
      event.putCustomAttribute(act, param != 0 ? "selection" : "all");
    } else if (action == Analytics.PLAYLIST_ACTION_MOVE) {
      //no param
    } else if (action == Analytics.PLAYLIST_ACTION_SORT) {
      //TODO: rework
      final ProviderClient.SortBy by = ProviderClient.SortBy.values()[param / 100];
      final ProviderClient.SortOrder order = ProviderClient.SortOrder.values()[param % 100];
      event.putCustomAttribute(act, by.name() + "/" + order.name());
    } else {
      event.putCustomAttribute(act, param != 0 ? "selection" : "global");
    }
    send(event);
  }

  private static String serializePlaylistAction(@Analytics.PlaylistAction int action) {
    switch (action) {
      case Analytics.PLAYLIST_ACTION_ADD:
        return "Add";
      case Analytics.PLAYLIST_ACTION_DELETE:
        return "Delete";
      case Analytics.PLAYLIST_ACTION_MOVE:
        return "Move";
      case Analytics.PLAYLIST_ACTION_SORT:
        return "Sort";
      case Analytics.PLAYLIST_ACTION_SAVE:
        return "Save";
      case Analytics.PLAYLIST_ACTION_STATISTICS:
        return "Statistics";
      default:
        return "";
    }
  }

  @Override
  public void sendVfsEvent(String id, String scope, int action) {
    final CustomEvent event = new CustomEvent("Vfs");
    final String type = serializeVfsAction(action);
    event.putCustomAttribute(id, type);
    event.putCustomAttribute(id + "/" + scope, type);
    send(event);
  }

  private static String serializeVfsAction(@Analytics.VfsAction int action) {
    switch (action) {
      case Analytics.VFS_ACTION_REMOTE_FETCH:
      case Analytics.VFS_ACTION_REMOTE_FALLBACK:
        return "remote";
      case Analytics.VFS_ACTION_CACHED_FETCH:
      case Analytics.VFS_ACTION_CACHED_FALLBACK:
        return "cache";
      default:
        return "";
    }
  }

  @Override
  public void sendNoTracksFoundEvent(Uri uri) {
    final String source = uri.getScheme();
    final String filename = uri.getLastPathSegment();
    final int extPos = filename.lastIndexOf('.');
    final String type = extPos != -1 ? filename.substring(extPos + 1) : "None";

    final CustomEvent event = new CustomEvent("Investigation");
    event.putCustomAttribute("NoTracksType", type);
    event.putCustomAttribute("NoTracksTypeDetailed", source + "/" + type);
    send(event);
  }


  @Override
  public void sendHostUnavailableEvent(String host) {
    final CustomEvent event = new CustomEvent("Investigation");
    event.putCustomAttribute("UnavailableHost", host);
    send(event);
  }

  private static void send(CustomEvent event) {
    Answers.getInstance().logCustom(event);
    Log.d(TAG, event.toString());
  }
}
