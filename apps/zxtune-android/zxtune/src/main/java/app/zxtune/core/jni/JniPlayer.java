package app.zxtune.core.jni;

import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Player;

public final class JniPlayer implements Player {

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final int handle;

  JniPlayer(int handle) throws Exception {
    this.handle = handle;
    JniGC.register(this, handle, getProperty(ModuleAttributes.TYPE, "Unknown"));
  }

  public static native void close(int handle);
  public static native int getPlaybackPerformance(int player) throws Exception;

  @Override
  public native boolean render(short[] result) throws Exception;

  @Override
  public native int analyze(int bands[], int levels[]) throws Exception;

  @Override
  public native int getPosition() throws Exception;

  @Override
  public native void setPosition(int pos) throws Exception;

  @Override
  public native long getProperty(String name, long defVal) throws Exception;

  @Override
  public native String getProperty(String name, String defVal) throws Exception;

  @Override
  public native void setProperty(String name, long val) throws Exception;

  @Override
  public native void setProperty(String name, String val) throws Exception;
}
