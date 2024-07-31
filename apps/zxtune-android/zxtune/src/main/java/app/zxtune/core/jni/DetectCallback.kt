package app.zxtune.core.jni

import app.zxtune.core.Module
import java.nio.ByteBuffer

interface DetectCallback {
    fun onModule(subPath: String, obj: Module)
    fun onPicture(subPath: String, obj: ByteArray)
}

fun interface DataCallback {
    fun onData(data: ByteBuffer)
}
