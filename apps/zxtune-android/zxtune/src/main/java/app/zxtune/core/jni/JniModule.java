package app.zxtune.core.jni;

import android.support.annotation.NonNull;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleDetectCallback;
import app.zxtune.core.Player;
import app.zxtune.core.ResolvingException;

import java.nio.ByteBuffer;

public final class JniModule implements Module {

  static {
    JniLibrary.load();
  }

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final int handle;

  JniModule(int handle) {
    this.handle = handle;
    JniGC.register(this, handle);
  }

  @SuppressWarnings("RedundantThrows")
  @NonNull
  public static native JniModule load(@NonNull ByteBuffer data, @NonNull String subpath) throws ResolvingException;

  public static native void detect(@NonNull ByteBuffer data, @NonNull ModuleDetectCallback callback);

  @Override
  public final void release() {
    close(handle);
  }

  static native void close(int handle);

  @Override
  public native int getDuration();

  @NonNull
  @Override
  public native Player createPlayer();

  @Override
  public native long getProperty(@NonNull String name, long defVal);

  @NonNull
  @Override
  public native String getProperty(@NonNull String name, @NonNull String defVal);

  @Override
  public native String[] getAdditionalFiles();

  @Override
  public native void resolveAdditionalFile(@NonNull String name, @NonNull ByteBuffer data);
}
