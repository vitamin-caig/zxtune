package app.zxtune;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.Nullable;

import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.answers.CustomEvent;
import com.crashlytics.android.ndk.CrashlyticsNdk;

import java.util.List;
import java.util.concurrent.TimeUnit;

import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlayableItem;
import io.fabric.sdk.android.Fabric;

public class Analytics {

  private static final String TAG = Analytics.class.getName();

  public static void initialize(Context ctx) {
    Fabric.with(ctx, new Crashlytics(), new CrashlyticsNdk(), new Answers());
  }

  public static void logException(Throwable e) {
    Crashlytics.logException(e);
  }

  public static class PlaybackEventsCallback implements Callback {

    private Item lastItem = null;

    @Override
    public void onStatusChanged(boolean isPlaying) {
      if (isPlaying && lastItem != null) {
        sendPlayEvent((PlayableItem) lastItem);
        lastItem = null;
      }
    }

    @Override
    public void onItemChanged(Item item) {
      lastItem = item;
    }

    @Override
    public void onIOStatusChanged(boolean isActive) {
    }

    @Override
    public void onError(String e) {
    }
  }

  private static void sendPlayEvent(PlayableItem item) {
    try {
      final Identifier id = item.getDataId();
      final Uri location = id.getFullLocation();
      final ZXTune.Module module = item.getModule();
      final String type = module.getProperty(ZXTune.Module.Attributes.TYPE, "Unknown");
      final String program = module.getProperty(ZXTune.Module.Attributes.PROGRAM, "Unknown");
      final String container = module.getProperty(ZXTune.Module.Attributes.CONTAINER, "None");
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
    } catch (Exception e) {
      Log.w(TAG, e, "sendPlayEvent");
    }
  }

  public static void sendBrowseEvent(VfsDir dir) {
    sendCommonBrowserEvent("Browse", dir);
  }

  public static void sendSearchEvent(VfsDir dir) {
    sendCommonBrowserEvent("Search", dir);
  }

  private static void sendCommonBrowserEvent(String type, VfsDir dir) {
    final Uri path = dir.getUri();
    final Identifier id = new Identifier(path);
    final int archiveDepth = id.getSubpathComponents().length;
    final int depth = path.getPathSegments().size() + archiveDepth;
    final boolean isArchive = archiveDepth != 0 || VfsArchive.checkIfArchive(dir);

    final CustomEvent event = new CustomEvent(type);
    fillSource(event, path);
    event.putCustomAttribute("Depth", depth)
            .putCustomAttribute("Type", isArchive ? "Archive" : "Folder")
    ;
    send(event);
  }

  public static void sendSocialEvent(String method, String app, Item item) {
    final Identifier id = item.getDataId();
    final Uri location = id.getFullLocation();
    final boolean fromBrowser = item.getId().equals(location);

    final CustomEvent event = new CustomEvent("Social");
    fillSource(event, location);
    event.putCustomAttribute("Method", method)
            .putCustomAttribute("Application", app)
            .putCustomAttribute("MethodDetailed", method + "/" + app)
            .putCustomAttribute("Library", fromBrowser ? "Browser" : "Playlist")
    ;
    send(event);
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

  public static void sendUIEvent(String action) {
    final CustomEvent event = new CustomEvent("UI");
    event.putCustomAttribute("Action", action);
    send(event);
  }

  public static void sendPlaylistEvent(String action, @Nullable Object parameter) {
    final CustomEvent event = new CustomEvent("Playlist");
    event.putCustomAttribute("Action", action);
    if (parameter instanceof String) {
      event.putCustomAttribute(action, (String) parameter);
    } else if (parameter instanceof Number) {
      event.putCustomAttribute(action, (Number) parameter);
    }
    send(event);
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

  public static void sendTooBigFileEvent(int size) {
    final CustomEvent event = new CustomEvent("Investigation");
    event.putCustomAttribute("FileSizeKb", size / 1024);
    send(event);
  }

  public static void setFile(Uri uri, String subpath, int size) {
    //obfuscate user-sensitive data (paths)
    final String location = "file".equals(uri.getScheme())
            ? uri.getLastPathSegment()
            : uri.toString();
    Crashlytics.setString("file.location", location);
    Crashlytics.setString("file.subpath", subpath);
    Crashlytics.setInt("file.size", size);
  }

  private static void send(CustomEvent event) {
    Answers.getInstance().logCustom(event);
    Log.d(TAG, event.toString());
  }
}
