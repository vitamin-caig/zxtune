package app.zxtune.core.jni;

import java.lang.annotation.Native;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
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
  public TimeStamp getPosition() {
    return TimeStamp.createFrom(getPositionMs(), TimeUnit.MILLISECONDS);
  }

  private native int getPositionMs();

  @Override
  public void setPosition(TimeStamp pos) {
    setPositionMs((int)pos.convertTo(TimeUnit.MILLISECONDS));
  }

  private native void setPositionMs(int pos);

  @Override
  public native int getPerformance();

  @Override
  public native int getProgress();

  @Override
  public native void setProperty(String name, long val);

  @Override
  public native void setProperty(String name, String val);
}
