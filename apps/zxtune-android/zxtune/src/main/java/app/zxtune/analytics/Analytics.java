package app.zxtune.analytics;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.IntDef;
import androidx.collection.SparseArrayCompat;

import com.crashlytics.android.Crashlytics;

import java.lang.annotation.Retention;

import app.zxtune.core.Player;
import app.zxtune.playback.PlayableItem;

import static java.lang.annotation.RetentionPolicy.SOURCE;

public class Analytics {

  private static Sink[] sinks = {};

  public static void initialize(Context ctx) {
    if (FabricSink.isEnabled()) {
      sinks = new Sink[]{new FabricSink(ctx), new InternalSink(ctx)};
    } else {
      sinks = new Sink[]{new InternalSink(ctx)};
    }
  }

  public static void logException(Throwable e) {
    for (Sink s : sinks) {
      s.logException(e);
    }
  }

  // TODO: replace by annotation
  public static class Trace {
    private final String id;
    private final long start = System.nanoTime();
    private final SparseArrayCompat<String> points = new SparseArrayCompat<>();
    private String method = "";

    public static Trace create(String id) {
      return new Trace(id);
    }

    private Trace(String id) {
      this.id = id;
    }

    public final void beginMethod(String name) {
      points.clear();
      method = name;
      checkpoint("in");
    }

    public final void checkpoint(String point) {
      points.append(getElapsed(), point);
    }

    public final void endMethod() {
      checkpoint("out");
      sendTrace(id + "." + method, points);
      method = "";
    }

    private int getElapsed() {
      return (int) ((System.nanoTime() - start) / 1000);
    }
  }

  private static void sendTrace(String id, SparseArrayCompat<String> points) {
    for (Sink s : sinks) {
      s.sendTrace(id, points);
    }
  }

  public static void setNativeCallTags(Uri uri, String subpath, int size) {
    if (FabricSink.isEnabled()) {
      Crashlytics.setString("file", "file".equals(uri.getScheme()) ? uri.getLastPathSegment() :
          uri.toString());
      Crashlytics.setString("subpath", subpath);
      Crashlytics.setInt("size", size);
    }
  }

  public static void sendPlayEvent(PlayableItem item, Player player) {
    for (Sink s : sinks) {
      s.sendPlayEvent(item, player);
    }
  }

  @Retention(SOURCE)
  @IntDef({BROWSER_ACTION_BROWSE, BROWSER_ACTION_BROWSE_PARENT, BROWSER_ACTION_SEARCH})
  @interface BrowserAction {}

  public static final int BROWSER_ACTION_BROWSE = 0;
  public static final int BROWSER_ACTION_BROWSE_PARENT = 1;
  public static final int BROWSER_ACTION_SEARCH = 2;

  public static void sendBrowserEvent(Uri path, @BrowserAction int action) {
    for (Sink s : sinks) {
      s.sendBrowserEvent(path, action);
    }
  }

  @Retention(SOURCE)
  @IntDef({SOCIAL_ACTION_RINGTONE, SOCIAL_ACTION_SHARE, SOCIAL_ACTION_SEND})
  @interface SocialAction {}

  public static final int SOCIAL_ACTION_RINGTONE = 0;
  public static final int SOCIAL_ACTION_SHARE = 1;
  public static final int SOCIAL_ACTION_SEND = 2;

  public static void sendSocialEvent(Uri path, String app, @SocialAction int action) {
    for (Sink s : sinks) {
      s.sendSocialEvent(path, app, action);
    }
  }

  @Retention(SOURCE)
  @IntDef({UI_ACTION_OPEN, UI_ACTION_CLOSE, UI_ACTION_PREFERENCES, UI_ACTION_RATE, UI_ACTION_ABOUT,
      UI_ACTION_QUIT})
  @interface UiAction {}

  public static final int UI_ACTION_OPEN = 0;
  public static final int UI_ACTION_CLOSE = 1;
  public static final int UI_ACTION_PREFERENCES = 2;
  public static final int UI_ACTION_RATE = 3;
  public static final int UI_ACTION_ABOUT = 4;
  public static final int UI_ACTION_QUIT = 5;

  public static void sendUiEvent(@UiAction int action) {
    for (Sink s : sinks) {
      s.sendUiEvent(action);
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

  public static void sendPlaylistEvent(@PlaylistAction int action, int param) {
    for (Sink s : sinks) {
      s.sendPlaylistEvent(action, param);
    }
  }

  @Retention(SOURCE)
  @IntDef({VFS_ACTION_REMOTE_FETCH, VFS_ACTION_REMOTE_FALLBACK, VFS_ACTION_CACHED_FETCH, VFS_ACTION_CACHED_FALLBACK})
  @interface VfsAction {}

  public static final int VFS_ACTION_REMOTE_FETCH = 0;
  public static final int VFS_ACTION_REMOTE_FALLBACK = 1;
  public static final int VFS_ACTION_CACHED_FETCH = 2;
  public static final int VFS_ACTION_CACHED_FALLBACK = 3;

  public static class VfsTrace {
    private final long start = System.currentTimeMillis();
    private final String id;
    private final String scope;

    private VfsTrace(String id, String scope) {
      this.id = id;
      this.scope = scope;
    }

    public static VfsTrace create(String id, String scope) {
      return new VfsTrace(id, scope);
    }

    public final void send(@VfsAction int action) {
      final long duration = System.currentTimeMillis() - start;
      sendVfsEvent(id, scope, action, duration);
    }
  }

  private static void sendVfsEvent(String id, String scope, @VfsAction int action, long duration) {
    for (Sink s : sinks) {
      s.sendVfsEvent(id, scope, action, duration);
    }
  }

  public static void sendNoTracksFoundEvent(Uri uri) {
    for (Sink s : sinks) {
      s.sendNoTracksFoundEvent(uri);
    }
  }
}
