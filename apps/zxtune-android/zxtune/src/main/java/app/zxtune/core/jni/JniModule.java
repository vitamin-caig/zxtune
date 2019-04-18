package app.zxtune.core.jni;

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

  public static native JniModule load(ByteBuffer data, String subpath) throws Exception;

  public static native void detect(ByteBuffer data, ModuleDetectCallback callback) throws Exception;

  public static native void close(int handle);

  @Override
  public native int getDuration() throws Exception;

  @Override
  public native Player createPlayer() throws Exception;

  @Override
  public native long getProperty(String name, long defVal) throws Exception;

  @Override
  public native String getProperty(String name, String defVal) throws Exception;

  @Override
  public native String[] getAdditionalFiles() throws Exception;

  @Override
  public native void resolveAdditionalFile(String name, ByteBuffer data) throws Exception;
}
