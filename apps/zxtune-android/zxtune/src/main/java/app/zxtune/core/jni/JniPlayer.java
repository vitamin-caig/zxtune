package app.zxtune.core.jni;

import android.support.annotation.NonNull;
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
  public native boolean render(@NonNull short[] result) throws Exception;

  @Override
  public native int analyze(@NonNull int bands[], @NonNull int levels[]) throws Exception;

  @Override
  public native int getPosition() throws Exception;

  @Override
  public native void setPosition(int pos) throws Exception;

  @Override
  public native long getProperty(@NonNull String name, long defVal) throws Exception;

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal) throws Exception;

  @Override
  public native void setProperty(@NonNull String name, long val) throws Exception;

  @Override
  public native void setProperty(@NonNull String name, @NonNull String val) throws Exception;
}
