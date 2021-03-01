package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import java.nio.ByteBuffer;

import app.zxtune.core.Module;
import app.zxtune.core.ResolvingException;
import app.zxtune.utils.ProgressCallback;

public class JniApi {

  static {
    JniLibrary.load();
  }

  public static native Module loadModule(ByteBuffer data, String subPath) throws ResolvingException;

  public static native void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress);

}
