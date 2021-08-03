package app.zxtune.analytics.internal

import java.io.IOException

interface UrlsSink {
    @Throws(IOException::class)
    fun push(url: String)
}
