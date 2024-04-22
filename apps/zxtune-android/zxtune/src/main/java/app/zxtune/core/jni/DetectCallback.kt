package app.zxtune.core.jni

import app.zxtune.core.Module

interface DetectCallback {
    fun onModule(subPath: String, obj: Module)
    fun onPicture(subPath: String, obj: ByteArray)
}
