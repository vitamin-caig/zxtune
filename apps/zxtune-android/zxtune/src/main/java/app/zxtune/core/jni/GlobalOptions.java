package app.zxtune.core.jni;

import app.zxtune.core.PropertiesContainer;

public class GlobalOptions implements PropertiesContainer {

  static {
    JniLibrary.load();
  }

  private GlobalOptions() {
  }

  @Override
  public native void setProperty(String name, long value);

  @Override
  public native void setProperty(String name, String value);

  @Override
  public native long getProperty(String name, long defVal);

  @Override
  public native String getProperty(String name, String defVal);

  //TODO: return PropertiesContainer after throws cleanup
  @SuppressWarnings("SameReturnValue")
  public static GlobalOptions instance() {
    return Holder.INSTANCE;
  }

  private static class Holder {
    static final GlobalOptions INSTANCE = new GlobalOptions();
  }
}
