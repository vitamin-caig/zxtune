package app.zxtune.core.jni

import androidx.annotation.Keep
import app.zxtune.core.Module
import java.nio.ByteBuffer

@Keep
interface DetectCallback {
    fun onModule(subPath: String, obj: Module)
    fun onPicture(subPath: String, obj: ByteArray)
}

@Keep
fun interface DataCallback {
    fun onData(data: ByteBuffer)
}
