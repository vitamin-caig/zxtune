package app.zxtune.core.jni;

import android.support.annotation.NonNull;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleDetectCallback;
import app.zxtune.core.Player;

import java.nio.ByteBuffer;

public final class JniModule implements Module {

  static {
    System.loadLibrary("zxtune");
  }

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final int handle;

  public JniModule(int handle) {
    this.handle = handle;
    JniGC.register(this, handle);
  }

  @NonNull
  public static native JniModule load(@NonNull ByteBuffer data, @NonNull String subpath) throws Exception;

  public static native void detect(@NonNull ByteBuffer data, @NonNull ModuleDetectCallback callback) throws Exception;

  public static native void close(int handle);

  @Override
  public native int getDuration() throws Exception;

  @NonNull
  @Override
  public native Player createPlayer() throws Exception;

  @Override
  public native long getProperty(@NonNull String name, long defVal) throws Exception;

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal) throws Exception;

  @Override
  public native String[] getAdditionalFiles() throws Exception;

  @Override
  public native void resolveAdditionalFile(@NonNull String name, @NonNull ByteBuffer data) throws Exception;
}
