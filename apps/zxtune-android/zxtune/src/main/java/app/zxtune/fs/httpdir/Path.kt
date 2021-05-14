package app.zxtune.fs.httpdir

import android.net.Uri

interface Path {
    fun getRemoteUris(): Array<Uri>
    fun getLocalId(): String
    fun getUri(): Uri
    fun getName(): String
    fun getParent(): Path?
    fun getChild(name: String): Path
    fun isEmpty(): Boolean
    fun isFile(): Boolean
}
