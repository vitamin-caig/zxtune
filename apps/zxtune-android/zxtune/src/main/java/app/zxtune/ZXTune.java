/**
 * @file
 * @brief Gate to native zxtune code and related types
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import java.nio.ByteBuffer;

import app.zxtune.core.Module;
import app.zxtune.core.Player;
import app.zxtune.core.jni.JniGC;

public final class ZXTune {

  /**
   * Simple data factory
   *
   * @param content raw content
   * @param subpath module subpath in content
   * @return New object
   */
  public static Module loadModule(ByteBuffer content, String subpath) throws Exception {
    return new NativeModule(Module_Create(makeDirectBuffer(content), subpath));
  }

  public interface ModuleDetectCallback {
    void onModule(String subpath, Module obj) throws Exception;
  }

  static class ModuleDetectCallbackNativeAdapter {

    private final ModuleDetectCallback delegate;

    public ModuleDetectCallbackNativeAdapter(ModuleDetectCallback delegate) {
      this.delegate = delegate;
    }

    final void onModule(String subpath, int handle) throws Exception {
      delegate.onModule(subpath, new NativeModule(handle));
    }
  }

  public static void detectModules(ByteBuffer content, ModuleDetectCallback cb) throws Exception {
    Module_Detect(makeDirectBuffer(content), new ModuleDetectCallbackNativeAdapter(cb));
  }

  private static ByteBuffer makeDirectBuffer(ByteBuffer content) throws Exception {
    if (content.position() != 0) {
      throw new Exception("Input data should have zero position");
    }
    if (content.isDirect()) {
      return content;
    } else {
      final ByteBuffer direct = ByteBuffer.allocateDirect(content.limit());
      direct.put(content);
      direct.position(0);
      content.position(0);
      return direct;
    }
  }

  private static final class NativeModule implements Module {

    private final int handle;

    NativeModule(int handle) {
      this.handle = handle;
      JniGC.register(this, handle);
    }

    @Override
    public int getDuration() throws Exception {
      return Module_GetDuration(handle);
    }

    @Override
    public Player createPlayer() throws Exception {
      return new NativePlayer(Module_CreatePlayer(handle));
    }

    @Override
    public long getProperty(String name, long defVal) throws Exception {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) throws Exception {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public String[] getAdditionalFiles() throws Exception {
      return Module_GetAdditionalFiles(handle);
    }

    @Override
    public void resolveAdditionalFile(String name, ByteBuffer data) throws Exception {
      Module_ResolveAdditionalFile(handle, name, makeDirectBuffer(data));
    }
  }

  private static final class NativePlayer implements Player {

    private final int handle;

    NativePlayer(int handle) {
      this.handle = handle;
      JniGC.register(this, handle);
    }

    @Override
    public boolean render(short[] result) throws Exception {
      return Player_Render(handle, result);
    }

    @Override
    public int analyze(int bands[], int levels[]) throws Exception {
      return Player_Analyze(handle, bands, levels);
    }

    @Override
    public int getPosition() throws Exception {
      return Player_GetPosition(handle);
    }

    @Override
    public void setPosition(int pos) throws Exception {
      Player_SetPosition(handle, pos);
    }

    @Override
    public long getProperty(String name, long defVal) throws Exception {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) throws Exception {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public void setProperty(String name, long val) throws Exception {
      Player_SetProperty(handle, name, val);
    }

    @Override
    public void setProperty(String name, String val) throws Exception {
      Player_SetProperty(handle, name, val);
    }
  }

  static {
    init();
  }

  public static final void init() {
    System.loadLibrary("zxtune");
  }

  // working with module
  private static native int Module_Create(ByteBuffer data, String subpath) throws Exception;

  private static native void Module_Detect(ByteBuffer data, ModuleDetectCallbackNativeAdapter cb) throws Exception;

  private static native int Module_GetDuration(int module) throws Exception;

  private static native long Module_GetProperty(int module, String name, long defVal) throws Exception;

  private static native String Module_GetProperty(int module, String name, String defVal) throws Exception;

  private static native int Module_CreatePlayer(int module) throws Exception;

  private static native String[] Module_GetAdditionalFiles(int module) throws Exception;

  private static native void Module_ResolveAdditionalFile(int module, String name, ByteBuffer data) throws Exception;

  public static native void Module_Close(int module);

  // working with player
  private static native boolean Player_Render(int player, short[] result) throws Exception;

  private static native int Player_Analyze(int player, int bands[], int levels[]) throws Exception;

  private static native int Player_GetPosition(int player) throws Exception;

  private static native void Player_SetPosition(int player, int pos) throws Exception;

  public  static native int Player_GetPlaybackPerformance(int player) throws Exception;

  private static native long Player_GetProperty(int player, String name, long defVal) throws Exception;

  public static native String Player_GetProperty(int player, String name, String defVal) throws Exception;

  private static native void Player_SetProperty(int player, String name, long val) throws Exception;

  private static native void Player_SetProperty(int player, String name, String val) throws Exception;

  public static native void Player_Close(int player);
}
