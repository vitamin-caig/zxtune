package app.zxtune.core.jni;

import app.zxtune.core.Module;

public interface DetectCallback {
  @SuppressWarnings({"unused"})
  void onModule(String subPath, Module obj);
}
