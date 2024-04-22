package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import java.nio.ByteBuffer;

import app.zxtune.Log;
import app.zxtune.core.Module;
import app.zxtune.core.PropertiesContainer;
import app.zxtune.core.ResolvingException;
import app.zxtune.utils.NativeLoader;
import app.zxtune.utils.ProgressCallback;

class JniApi implements Api {

  private static final String TAG = JniApi.class.getName();

  private static final String LIBRARY_NAME = "zxtune";

  private final PropertiesContainer loggingOptions;

  JniApi() {
    NativeLoader.loadLibrary(LIBRARY_NAME, this::forcedInit);
    loggingOptions = new LoggingOptionsAdapter(JniOptions.instance());
  }

  private native void forcedInit();

  @Override
  public native Module loadModule(ByteBuffer data, String subPath) throws ResolvingException;

  @Override
  public native void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress);

  @Override
  public native void enumeratePlugins(Plugins.Visitor visitor);

  public static void loadLibraryForTest() {
    System.loadLibrary(LIBRARY_NAME);
  }

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
    @Nullable
    public byte[] getProperty(String name, @Nullable byte[] defVal) {
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
