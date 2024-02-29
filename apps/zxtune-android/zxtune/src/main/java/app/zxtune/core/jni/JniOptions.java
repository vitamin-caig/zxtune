package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import app.zxtune.core.PropertiesContainer;

final class JniOptions implements PropertiesContainer {

  private JniOptions() {
  }

  @Override
  public native void setProperty(String name, long value);

  @Override
  public native void setProperty(String name, String value);

  @Override
  public native long getProperty(String name, long defVal);

  @Override
  public native String getProperty(String name, String defVal);

  @Override
  @Nullable
  public byte[] getProperty(String name, @Nullable byte[] defVal) {
    return defVal;
  }

  @SuppressWarnings("SameReturnValue")
  static PropertiesContainer instance() {
    return Holder.INSTANCE;
  }

  private static class Holder {
    static final JniOptions INSTANCE = new JniOptions();
  }
}
