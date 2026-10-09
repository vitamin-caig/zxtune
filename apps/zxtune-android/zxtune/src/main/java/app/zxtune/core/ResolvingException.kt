package app.zxtune.core

import androidx.annotation.Keep

@Keep
class ResolvingException : Exception {
    internal constructor(msg: String) : super(msg)
    internal constructor(msg: String, e: Throwable) : super(msg, e)

    companion object {
        private const val serialVersionUID = 1L
    }
}
