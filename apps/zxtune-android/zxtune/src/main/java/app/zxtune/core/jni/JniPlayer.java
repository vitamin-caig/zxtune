package app.zxtune.core.jni;

import androidx.annotation.NonNull;

import app.zxtune.core.Player;

public final class JniPlayer implements Player {

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
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
  public native boolean render(@NonNull short[] result);

  @Override
  public native int analyze(@NonNull byte levels[]);

  @Override
  public native int getPosition();

  @Override
  public native void setPosition(int pos);

  @Override
  public native int getPerformance();

  @Override
  public native int getProgress();

  @Override
  public native long getProperty(@NonNull String name, long defVal);

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal);

  @Override
  public native void setProperty(@NonNull String name, long val);

  @Override
  public native void setProperty(@NonNull String name, @NonNull String val);
}
