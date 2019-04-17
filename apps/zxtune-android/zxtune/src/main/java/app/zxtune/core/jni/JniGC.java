package app.zxtune.core.jni;

import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.ZXTune;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Player;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

//TODO: make package-private
public class JniGC {

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

  public static void register(Module owner, int handle) {
    Holder.instance.register(owner, handle, true);
  }

  public static void register(Player owner, int handle) {
    Holder.instance.register(owner, handle, false);
  }

  private void register(Object owner, int handle, boolean isModule) {
    final HandleReference ref = new HandleReference(deadRefs, owner, handle, isModule);
    handles.add(ref);
  }

  private static class Holder {
    private static final JniGC instance = new JniGC();
  }

  private static class HandleReference extends WeakReference<Object> {

    private final int handle;
    private final boolean isModule;

    private HandleReference(ReferenceQueue<? super Object> queue, Object owner, int handle, boolean isModule) {
      super(owner, queue);
      this.handle = handle;
      this.isModule = isModule;
    }

    private void destroy() {
      if (isModule) {
        ZXTune.Module_Close(handle);
      } else {
        sendPlayerStatistics();
        ZXTune.Player_Close(handle);
      }
    }

    private void sendPlayerStatistics() {
      try {
        Analytics.sendPerformanceEvent(ZXTune.Player_GetPlaybackPerformance(handle), ZXTune.Player_GetProperty(handle,
            ModuleAttributes.TYPE, "Unknown"));
      } catch (Exception e) {
        Log.w(ZXTune.class.getName(), e, "Failed to send player statistics");
      }
    }
  }
}
