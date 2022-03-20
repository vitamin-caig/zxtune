package app.zxtune.fs.http

import android.net.Uri
import java.io.IOException
import java.io.InputStream

interface HttpProvider {
    fun hasConnection(): Boolean

    @Throws(IOException::class)
    fun getObject(uri: Uri): HttpObject

    @Throws(IOException::class)
    fun getInputStream(uri: Uri): InputStream
}