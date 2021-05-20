package app.zxtune.fs.http

import android.net.Uri
import app.zxtune.TimeStamp
import java.io.IOException
import java.io.InputStream

interface HttpObject {

    val uri: Uri

    val contentLength: Long?

    val lastModified: TimeStamp?

    @get:Throws(IOException::class)
    val input: InputStream
}
