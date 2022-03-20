package app.zxtune.core.jni

import androidx.annotation.Keep

@Keep
@Suppress("unused")
class JniRuntimeException : RuntimeException {
    internal constructor(msg: String) : super(msg)
    internal constructor(msg: String, e: Throwable) : super(msg, e)
}
