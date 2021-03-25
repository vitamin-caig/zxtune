package app.zxtune.utils;

import android.os.Debug;

import app.zxtune.analytics.Analytics;

// TODO: rework metrics retrieval
public class NativeLoader {

  public static void loadLibrary(String name, Runnable initialize) {
    final Traces trace = new Traces("JNI");
    try {
      trace.beginMethod(name);
      System.loadLibrary(name);
      trace.checkpoint("load");
      initialize.run();
    } catch (Exception e) {
      trace.checkpoint("fail");
      throw new RuntimeException("Failed to load library " + name, e);
    } finally {
      trace.endMethod();
    }
  }

  private static class NativeHeapTrace extends Analytics.BaseTrace {
    NativeHeapTrace(String id) {
      super(id);
    }

    @Override
    protected long getMetric() {
      return Debug.getNativeHeapAllocatedSize();
    }
  }

  private static class Traces {

    private final Analytics.BaseTrace[] traces;

    Traces(String id) {
      traces = new Analytics.BaseTrace[]{
          Analytics.Trace.create(id + "-times"),
          new NativeHeapTrace(id + "-nativeheap")
      };
    }

    final void beginMethod(String method) {
      for (Analytics.BaseTrace t : traces) {
        t.beginMethod(method);
      }
    }

    final void checkpoint(String id) {
      for (Analytics.BaseTrace t : traces) {
        t.checkpoint(id);
      }
    }

    final void endMethod() {
      for (Analytics.BaseTrace t : traces) {
        t.endMethod();
      }
    }
  }
}
