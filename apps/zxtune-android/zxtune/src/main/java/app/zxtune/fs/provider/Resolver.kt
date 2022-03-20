package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.VfsObject
import app.zxtune.utils.ProgressCallback

internal interface Resolver {
    fun resolve(uri: Uri): VfsObject?
    fun resolve(uri: Uri, cb: ProgressCallback): VfsObject?
}
