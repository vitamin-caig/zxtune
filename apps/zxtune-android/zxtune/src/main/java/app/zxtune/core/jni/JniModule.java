package app.zxtune.core.jni;

import app.zxtune.ZXTune;
import app.zxtune.core.Module;
import app.zxtune.core.Player;
import app.zxtune.jni.core.JniGC;

import java.nio.ByteBuffer;

public final class JniModule implements Module {

  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final int handle;

  public JniModule(int handle) {
    this.handle = handle;
    JniGC.register(this, handle);
  }

  public static native void close(int handle);

  @Override
  public native int getDuration() throws Exception;

  @Override
  public Player createPlayer() throws Exception {
    return new ZXTune.NativePlayer(createPlayerInternal());
  }

  private native int createPlayerInternal() throws Exception;

  @Override
  public native long getProperty(String name, long defVal) throws Exception;

  @Override
  public native String getProperty(String name, String defVal) throws Exception;

  @Override
  public native String[] getAdditionalFiles() throws Exception;

  @Override
  public void resolveAdditionalFile(String name, ByteBuffer data) throws Exception {
    resolveAdditionalFileInternal(name, ZXTune.makeDirectBuffer(data));
  }

  private native void resolveAdditionalFileInternal(String name, ByteBuffer data) throws Exception;
}
