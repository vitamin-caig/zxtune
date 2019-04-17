/**
 * @file
 * @brief Gate to native zxtune code and related types
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import app.zxtune.core.Module;
import app.zxtune.core.jni.JniModule;

import java.nio.ByteBuffer;

public final class ZXTune {

  /**
   * Simple data factory
   *
   * @param content raw content
   * @param subpath module subpath in content
   * @return New object
   */
  public static Module loadModule(ByteBuffer content, String subpath) throws Exception {
    return new JniModule(Module_Create(makeDirectBuffer(content), subpath));
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
      delegate.onModule(subpath, new JniModule(handle));
    }
  }

  public static void detectModules(ByteBuffer content, ModuleDetectCallback cb) throws Exception {
    Module_Detect(makeDirectBuffer(content), new ModuleDetectCallbackNativeAdapter(cb));
  }

  public static ByteBuffer makeDirectBuffer(ByteBuffer content) throws Exception {
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

  static {
    init();
  }

  public static void init() {
    System.loadLibrary("zxtune");
  }

  // working with module
  private static native int Module_Create(ByteBuffer data, String subpath) throws Exception;

  private static native void Module_Detect(ByteBuffer data, ModuleDetectCallbackNativeAdapter cb) throws Exception;
}
