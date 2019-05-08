package app.zxtune.core.jni;

import app.zxtune.Analytics;

//should be single entrypoint to lock on
final class JniLibrary {

  private final static long loadTime;
  private static int calls;

  static {
    final long start = System.currentTimeMillis();
    System.loadLibrary("zxtune");
    loadTime = System.currentTimeMillis() - start;
  }

  synchronized static void load() {
    if (++calls == 1) {
      Analytics.sendJniLoadEvent(loadTime);
    }
  }
}
