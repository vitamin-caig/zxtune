package app.zxtune.core.jni;

import android.support.annotation.NonNull;
import app.zxtune.core.PropertiesContainer;

public class GlobalOptions implements PropertiesContainer {

  private GlobalOptions() {
    System.loadLibrary("zxtune");
  }

  @Override
  public native void setProperty(@NonNull String name, long value);

  @Override
  public native void setProperty(@NonNull String name, @NonNull String value);

  @Override
  public native long getProperty(@NonNull String name, long defVal);

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal);

  //TODO: return PropertiesContainer after throws cleanup
  @SuppressWarnings("SameReturnValue")
  @NonNull
  public static GlobalOptions instance() {
    return Holder.INSTANCE;
  }

  private static class Holder {
    static final GlobalOptions INSTANCE = new GlobalOptions();
  }
}
