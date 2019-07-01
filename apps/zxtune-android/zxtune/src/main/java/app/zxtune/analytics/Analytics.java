package app.zxtune.analytics;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.IntDef;
import app.zxtune.BuildConfig;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Player;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.PlaylistControl;
import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.answers.CustomEvent;
import com.crashlytics.android.core.CrashlyticsCore;
import com.crashlytics.android.ndk.CrashlyticsNdk;
import io.fabric.sdk.android.Fabric;

import java.lang.annotation.Retention;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import static java.lang.annotation.RetentionPolicy.SOURCE;

public class Analytics {

  private static final String TAG = Analytics.class.getName();

  public static void initialize(Context ctx) {
    if (BuildConfig.BUILD_TYPE.equals("release")) {
      Fabric.with(ctx, new Crashlytics(), new CrashlyticsNdk(), new Answers());
    } else {
      final Crashlytics crashlytics = new Crashlytics.Builder()
                                          .core(new CrashlyticsCore.Builder().disabled(true).build())
                                          .build();
      Fabric.with(ctx, crashlytics);
    }
  }

  public static void logException(Throwable e) {
    Crashlytics.logException(e);
  }

  public static void sendPlayEvent(PlayableItem item, Player player) {
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

  @Retention(SOURCE)
  @IntDef({BROWSER_ACTION_BROWSE, BROWSER_ACTION_SEARCH})
  @interface BrowserAction {}

  public static final int BROWSER_ACTION_BROWSE = 0;
  public static final int BROWSER_ACTION_SEARCH = 1;

  public static void sendBrowserEvent(Uri path, @BrowserAction int action) {
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

  private static String serializeBrowserAction(@BrowserAction int action) {
    switch (action) {
      case BROWSER_ACTION_BROWSE:
        return "Browse";
      case BROWSER_ACTION_SEARCH:
        return "Search";
      default:
        return "";
    }
  }

  @Retention(SOURCE)
  @IntDef({SOCIAL_ACTION_RINGTONE, SOCIAL_ACTION_SHARE, SOCIAL_ACTION_SEND})
  @interface SocialAction {}

  public static final int SOCIAL_ACTION_RINGTONE = 0;
  public static final int SOCIAL_ACTION_SHARE = 1;
  public static final int SOCIAL_ACTION_SEND = 2;

  public static void sendSocialEvent(Uri path, String app, @SocialAction int action) {
    final CustomEvent event = new CustomEvent("Social");
    fillSource(event, path);
    final String method = serializeSocialAction(action);
    event.putCustomAttribute("Method", method)
        .putCustomAttribute("Application", app)
        .putCustomAttribute("MethodDetailed", method + "/" + app)
    ;
    send(event);
  }

  private static String serializeSocialAction(@SocialAction int action) {
    switch (action) {
      case SOCIAL_ACTION_RINGTONE:
        return "Ringtone";
      case SOCIAL_ACTION_SHARE:
        return "Share";
      case SOCIAL_ACTION_SEND:
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

  @Retention(SOURCE)
  @IntDef({UI_ACTION_PREFERENCES, UI_ACTION_RATE, UI_ACTION_ABOUT})
  @interface UiAction {}

  public static final int UI_ACTION_PREFERENCES = 0;
  public static final int UI_ACTION_RATE = 1;
  public static final int UI_ACTION_ABOUT = 2;

  public static void sendUiEvent(@UiAction int action) {
    final CustomEvent event = new CustomEvent("UI");
    event.putCustomAttribute("Action", serializeUiAction(action));
    send(event);
  }

  private static String serializeUiAction(@UiAction int action) {
    switch (action) {
      case UI_ACTION_PREFERENCES:
        return "Preferences";
      case UI_ACTION_RATE:
        return "Rate";
      case UI_ACTION_ABOUT:
        return "About";
      default:
        return "";
    }
  }

  @Retention(SOURCE)
  @IntDef({PLAYLIST_ACTION_ADD, PLAYLIST_ACTION_DELETE, PLAYLIST_ACTION_MOVE, PLAYLIST_ACTION_SORT,
      PLAYLIST_ACTION_SAVE,
      PLAYLIST_ACTION_STATISTICS})
  @interface PlaylistAction {}

  public static final int PLAYLIST_ACTION_ADD = 0;
  public static final int PLAYLIST_ACTION_DELETE = 1;
  public static final int PLAYLIST_ACTION_MOVE = 2;
  public static final int PLAYLIST_ACTION_SORT = 3;
  public static final int PLAYLIST_ACTION_SAVE = 4;
  public static final int PLAYLIST_ACTION_STATISTICS = 5;

  public static void sendPlaylistEvent(@PlaylistAction int action, int scopeSize) {
    final CustomEvent event = new CustomEvent("Playlist");
    final String act = serializePlaylistAction(action);
    event.putCustomAttribute("Action", act);
    if (action == PLAYLIST_ACTION_ADD) {
      event.putCustomAttribute(act, scopeSize);
    } else if (action == PLAYLIST_ACTION_SAVE) {
      //compatibility
      event.putCustomAttribute(act, scopeSize != 0 ? "selection" : "all");
    } else if (action == PLAYLIST_ACTION_MOVE) {
      //no param
    } else if (action == PLAYLIST_ACTION_SORT) {
      //TODO: rework
      final PlaylistControl.SortBy by = PlaylistControl.SortBy.values()[scopeSize / 100];
      final PlaylistControl.SortOrder order = PlaylistControl.SortOrder.values()[scopeSize % 100];
      event.putCustomAttribute(act, by.name() + "/" + order.name());
    } else {
      event.putCustomAttribute(act, scopeSize != 0 ? "selection" : "global");
    }
    send(event);
  }

  private static String serializePlaylistAction(@PlaylistAction int action) {
    switch (action) {
      case PLAYLIST_ACTION_ADD:
        return "Add";
      case PLAYLIST_ACTION_DELETE:
        return "Delete";
      case PLAYLIST_ACTION_MOVE:
        return "Move";
      case PLAYLIST_ACTION_SORT:
        return "Sort";
      case PLAYLIST_ACTION_SAVE:
        return "Save";
      case PLAYLIST_ACTION_STATISTICS:
        return "Statistics";
      default:
        return "";
    }
  }

  public static void sendVfsRemoteEvent(String id, String scope) {
    sendVfsEvent(id, scope, "remote");
  }

  public static void sendVfsCacheEvent(String id, String scope) {
    sendVfsEvent(id, scope, "cache");
  }

  private static void sendVfsEvent(String id, String scope, String type) {
    final CustomEvent event = new CustomEvent("Vfs");
    event.putCustomAttribute(id, type);
    event.putCustomAttribute(id + "/" + scope, type);
    send(event);
  }

  public static void sendNoTracksFoundEvent(Uri uri) {
    final String source = uri.getScheme();
    final String filename = uri.getLastPathSegment();
    final int extPos = filename.lastIndexOf('.');
    final String type = extPos != -1 ? filename.substring(extPos + 1) : "None";
    Analytics.sendNoTracksFoundEvent(source, type);
  }

  public static void sendNoTracksFoundEvent(String source, String type) {
    final CustomEvent event = new CustomEvent("Investigation");
    event.putCustomAttribute("NoTracksType", type);
    event.putCustomAttribute("NoTracksTypeDetailed", source + "/" + type);
    send(event);
  }

  public static void sendHostUnavailableEvent(String host) {
    final CustomEvent event = new CustomEvent("Investigation");
    event.putCustomAttribute("UnavailableHost", host);
    send(event);
  }

  public static class JniLog {
    private final String prefix;

    public JniLog(Uri uri, String subpath, int size) {
      this.prefix = String.format(Locale.US, "%d: ", uri.hashCode() ^ subpath.hashCode());
      Crashlytics.log(prefix + String.format(Locale.US, "file=%s subpath=%s size=%d",
          "file".equals(uri.getScheme()) ? uri.getLastPathSegment() : uri.toString(),
          subpath, size));
    }

    public final void action(String action) {
      Crashlytics.log(prefix + action);
    }
  }

  private static void send(CustomEvent event) {
    Answers.getInstance().logCustom(event);
    Log.d(TAG, event.toString());
  }
}
