package app.zxtune.core.jni;

import android.support.annotation.Nullable;
import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.core.Module;
import app.zxtune.core.Player;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

class JniGC {

  private final ReferenceQueue<Object> deadRefs = new ReferenceQueue<>();
  private final ArrayList<HandleReference> handles = new ArrayList<>(10);
  private final Thread thread = new Thread("JNICleanup") {
    @Override
    public void run() {
      cleanup();
    }
  };

  private JniGC() {
    thread.setDaemon(true);
    thread.start();
  }

  private void cleanup() {
    while (true) {
      try {
        final HandleReference item = (HandleReference) deadRefs.remove();
        handles.remove(item);
        item.destroy();
        item.clear();
      } catch (InterruptedException e) {
      }
    }
  }

  static void register(Module owner, int handle) {
    Holder.instance.registerInternal(owner, handle, null);
  }

  static void register(Player owner, int handle, String type) {
    Holder.instance.registerInternal(owner, handle, type);
  }

  private void registerInternal(Object owner, int handle, String type) {
    final HandleReference ref = new HandleReference(deadRefs, owner, handle, type);
    handles.add(ref);
  }

  private static class Holder {
    private static final JniGC instance = new JniGC();
  }

  private static class HandleReference extends WeakReference<Object> {

    private final int handle;
    private final String playerType;

    private HandleReference(ReferenceQueue<? super Object> queue, Object owner, int handle, @Nullable String playerType) {
      super(owner, queue);
      this.handle = handle;
      this.playerType = playerType;
    }

    private void destroy() {
      if (playerType != null) {
        sendPlayerStatistics();
        JniPlayer.close(handle);
      } else {
        JniModule.close(handle);
      }
    }

    private void sendPlayerStatistics() {
      try {
        Analytics.sendPerformanceEvent(JniPlayer.getPlaybackPerformance(handle), playerType);
      } catch (Throwable e) {
        Log.w("app.zxtune.ZXTune", e, "Failed to send player statistics");
      }
    }
  }
}
