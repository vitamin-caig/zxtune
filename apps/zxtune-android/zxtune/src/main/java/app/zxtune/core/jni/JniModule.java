package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import java.lang.annotation.Native;
import java.nio.ByteBuffer;

import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.Player;

final class JniModule implements Module {

  @Native
  private final int handle;

  JniModule(int handle) {
    this.handle = handle;
    JniGC.register(this, handle);
  }

  @Override
  public void release() {
    close(handle);
  }

  static native void close(int handle);

  @Override
  public TimeStamp getDuration() {
    return TimeStamp.fromMilliseconds(getDurationMs());
  }

  private native int getDurationMs();

  @Override
  public native Player createPlayer(int samplerate);

  @Override
  public native long getProperty(String name, long defVal);

  @Override
  public native String getProperty(String name, String defVal);

  @Override
  @Nullable
  public native byte[] getProperty(String name, @Nullable byte[] defVal);

  @Override
  public native String[] getAdditionalFiles();

  @Override
  public native void resolveAdditionalFile(String name, ByteBuffer data);
}
