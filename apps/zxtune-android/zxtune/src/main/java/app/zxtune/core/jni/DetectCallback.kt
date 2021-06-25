package app.zxtune.core.jni

import app.zxtune.core.Module

fun interface DetectCallback {
  fun onModule(subPath: String, obj: Module)
}
