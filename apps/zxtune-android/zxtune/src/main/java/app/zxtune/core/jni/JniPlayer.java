package app.zxtune.core.jni;

import java.lang.annotation.Native;

import app.zxtune.core.Player;

public final class JniPlayer implements Player {

  @Native
  private final int handle;

  JniPlayer(int handle) {
    this.handle = handle;
    JniGC.register(this, handle);
  }

  @Override
  public final void release() {
    close(handle);
  }

  static native void close(int handle);

  @Override
  public native boolean render(short[] result);

  @Override
  public native int analyze(byte[] levels);

  @Override
  public native int getPosition();

  @Override
  public native void setPosition(int pos);

  @Override
  public native int getPerformance();

  @Override
  public native int getProgress();

  @Override
  public native long getProperty(String name, long defVal);

  @Override
  public native String getProperty(String name, String defVal);

  @Override
  public native void setProperty(String name, long val);

  @Override
  public native void setProperty(String name, String val);
}
