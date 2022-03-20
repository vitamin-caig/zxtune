package app.zxtune.analytics.internal

import android.net.Uri
import androidx.annotation.VisibleForTesting

class UrlsBuilder @VisibleForTesting internal constructor(type: String, ts: Long) {

    private val delegate: StringBuilder = StringBuilder(512)

    init {
        delegate.apply {
            append(type)
            append("?ts=")
            append(ts)
        }
    }

    constructor(type: String) : this(type, System.currentTimeMillis() / 1000)

  fun addParam(key: String, value: String?) {
        if (!value.isNullOrEmpty()) {
          addDelimiter().apply {
            append(key)
            append('=')
            append(Uri.encode(value))
          }
        }
    }

    fun addParam(key: String, value: Long) {
        if (value != DEFAULT_LONG_VALUE) {
            addDelimiter().apply {
              append(key)
              append('=')
              append(value)
            }
        }
    }

    fun addUri(uri: Uri) = addParam("uri", cleanupUri(uri))

    val result: String
        get() = delegate.toString()

    private fun addDelimiter() = delegate.append('&')

    private fun cleanupUri(uri: Uri) = when (val scheme = uri.scheme) {
        null -> "root"
        "file" -> scheme
        else -> uri.toString()
    }

    companion object {
        const val DEFAULT_STRING_VALUE = ""
        const val DEFAULT_LONG_VALUE: Long = -1
    }
}
