package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import java.nio.ByteBuffer;

import app.zxtune.Log;
import app.zxtune.analytics.Analytics;
import app.zxtune.core.Module;
import app.zxtune.core.PropertiesContainer;
import app.zxtune.core.ResolvingException;
import app.zxtune.utils.ProgressCallback;

class JniApi extends Api {

  private static final String TAG = JniApi.class.getName();

  /**
   * TODO: extract native libraries loading+init to separate service and trace per-library timings
   */
  private static final Analytics.Trace TRACE = Analytics.Trace.create("JNI");

  private final PropertiesContainer loggingOptions;

  JniApi() {
    TRACE.beginMethod("zxtune");
    System.loadLibrary("zxtune");
    TRACE.checkpoint("load");
    forcedInit();
    TRACE.endMethod();
    loggingOptions = new LoggingOptionsAdapter(JniOptions.instance());
  }

  private void forcedInit() {
    enumeratePlugins(new Plugins.Visitor() {
      @Override
      public void onPlayerPlugin(int devices, String id, String description) {
      }

      @Override
      public void onContainerPlugin(int type, String id, String description) {
      }
    });
  }

  @Override
  public native Module loadModule(ByteBuffer data, String subPath) throws ResolvingException;

  @Override
  public native void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress);

  @Override
  public native void enumeratePlugins(Plugins.Visitor visitor);

  @Override
  public PropertiesContainer getOptions() {
    return loggingOptions;
  }

  private static class LoggingOptionsAdapter implements PropertiesContainer {
    private final PropertiesContainer delegate;

    LoggingOptionsAdapter(PropertiesContainer delegate) {
      this.delegate = delegate;
    }

    @Override
    public long getProperty(String name, long defVal) {
      return delegate.getProperty(name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) {
      return delegate.getProperty(name, defVal);
    }

    @Override
    public void setProperty(String name, long value) {
      Log.d(TAG, "setProperty(%s, %d)", name, value);
      delegate.setProperty(name, value);
    }

    @Override
    public void setProperty(String name, String value) {
      Log.d(TAG, "setProperty(%s, '%s')", name, value);
      delegate.setProperty(name, value);
    }
  }
}
