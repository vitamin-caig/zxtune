package app.zxtune;

import android.content.Context;
import android.net.Uri;

import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.answers.CustomEvent;
import com.crashlytics.android.ndk.CrashlyticsNdk;

import java.util.List;

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
    final Identifier id = item.getDataId();
    final Uri location = id.getFullLocation();
    final ZXTune.Module module = item.getModule();
    final String type = module.getProperty(ZXTune.Module.Attributes.TYPE, "Unknown");
    final String program = module.getProperty(ZXTune.Module.Attributes.PROGRAM, "Unknown");
    final String container = module.getProperty(ZXTune.Module.Attributes.CONTAINER, "None");
    final boolean fromBrowser = item.getId().equals(location);

    final CustomEvent event = new CustomEvent("Play");
    fillSource(event, location);
    event.putCustomAttribute("Type", type)
            .putCustomAttribute("TypeDetailed", type + "/" + program)
            .putCustomAttribute("Container", container)
            .putCustomAttribute("Library", fromBrowser ? "Browser" : "Playlist")
    ;
    send(event);
  }

  public static void sendBrowseEvent(VfsDir dir) {
    sendCommonBrowserEvent("Browse", dir);
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

  private static void send(CustomEvent event) {
    Answers.getInstance().logCustom(event);
    Log.d(TAG, event.toString());
  }
}
