package app.zxtune.fs.hvsc

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.httpdir.PathBase

/*
 * 1) hvsc:/C64Music/${FilePath} - deprecated, compatibility
 * 2) hvsc:/${FilePath} - starting from C64Music directory
 */
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris() = getRemoteId().let { path ->
        arrayOf(
            Cdn.hvsc(path),
            Uri.Builder()
                .scheme("https")
                .authority("www.prg.dtu.dk")
                .path("HVSC/C64Music/${path}")
                .build(),
        )
    }

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        if (elements.isEmpty()) EMPTY else Path(elements, isDir)

    companion object {
        const val REMOTE_URLS_COUNT = 2

        private const val SCHEME = "hvsc"
        private const val OLD_PREFIX = "/C64Music/"

        private val EMPTY = Path(emptyList(), true).also {
            check(it.getRemoteUris().size == REMOTE_URLS_COUNT)
        }

        private fun stripOldPrefix(path: String?) =
            if (true == path?.startsWith(OLD_PREFIX)) {
                path.substring(OLD_PREFIX.length - 1)
            } else {
                path
            }

        @JvmStatic
        fun create() = EMPTY

        @JvmStatic
        fun parse(uri: Uri) =
            if (SCHEME == uri.scheme) {
                stripOldPrefix(uri.path)?.let { EMPTY.getChild(it) as Path } ?: EMPTY
            } else {
                null
            }
    }
}
