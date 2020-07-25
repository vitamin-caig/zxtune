package app.zxtune.core.jni;

import app.zxtune.core.Player;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

class JniGC {

  private final ReferenceQueue<Object> deadRefs = new ReferenceQueue<>();
  @SuppressWarnings("MismatchedQueryAndUpdateOfCollection")
  private final ArrayList<HandleReference> handles = new ArrayList<>(1000);
  private final Thread thread = new Thread("JNICleanup") {
    @Override
    public void run() {
      cleanup();
    }
  };
  private int registeredObjects = 0;
  private int destroyedObjects = 0;

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
        ++destroyedObjects;
      } catch (InterruptedException e) {
      }
    }
  }

  static void register(Object owner, int handle) {
    Holder.instance.registerInternal(owner, handle);
  }

  private void registerInternal(Object owner, int handle) {
    final HandleReference ref = new HandleReference(deadRefs, owner, handle);
    handles.add(ref);
    ++registeredObjects;
  }

  private static class Holder {
    private static final JniGC instance = new JniGC();
  }

  private static class HandleReference extends WeakReference<Object> {

    private final int handle;
    private final boolean isPlayer;

    private HandleReference(ReferenceQueue<? super Object> queue, Object owner, int handle) {
      super(owner, queue);
      this.handle = handle;
      this.isPlayer = owner instanceof Player;
    }

    private void destroy() {
      if (isPlayer) {
        JniPlayer.close(handle);
      } else {
        JniModule.close(handle);
      }
    }
  }
}
