package app.zxtune.fs

import android.net.Uri
import app.zxtune.utils.ProgressCallback
import org.mockito.kotlin.mock
import org.robolectric.annotation.Implements

@Implements(VfsArchive::class)
class ShadowVfsArchive {
    companion object {
        val doResolve = mock<(Uri, ProgressCallback?) -> VfsObject?>()

        @JvmStatic
        fun resolveForced(uri: Uri, cb: ProgressCallback) = doResolve(uri, cb)

        @JvmStatic
        fun resolve(uri: Uri) = doResolve(uri, null)

        val doGetModulesCount = mock<(Array<Uri>) -> Array<Int?>>()

        @JvmStatic
        fun getModulesCount(uris: Array<Uri>) = doGetModulesCount(uris)
    }
}
