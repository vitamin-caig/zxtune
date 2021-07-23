package app.zxtune.fs.dbhelpers

import app.zxtune.fs.http.HttpObject
import java.io.File
import java.io.IOException

interface DownloadCommand {
    @get:Throws(IOException::class)
    val cache: File

    @get:Throws(IOException::class)
    val remote: HttpObject
}
