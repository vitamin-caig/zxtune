package app.zxtune.fs.asma

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.httpdir.PathBase

/*
 * 1) asma:/${FilePath} - direct access to files or folders starting from ASMA's root (e.g. Composers)
 */
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris() = getRemoteId().let { path ->
        arrayOf(
            Cdn.asma(path),
            Uri.Builder()
                .scheme("https")
                .authority("asma.atari.org")
                .path("asma/${path}")
                .build()
        )
    }

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        if (elements.isEmpty()) EMPTY else Path(elements, isDir)

    companion object {
        const val REMOTE_URLS_COUNT = 2

        private const val SCHEME = "asma"

        private val EMPTY = Path(emptyList(), true).also {
            check(it.getRemoteUris().size == REMOTE_URLS_COUNT)
        }

        @JvmStatic
        fun create() = EMPTY

        @JvmStatic
        fun parse(uri: Uri) =
            if (SCHEME == uri.scheme) {
                uri.path?.let { EMPTY.getChild(it) as Path } ?: EMPTY
            } else {
                null
            }
    }
}
