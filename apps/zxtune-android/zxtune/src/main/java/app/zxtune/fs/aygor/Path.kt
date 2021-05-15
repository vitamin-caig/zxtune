package app.zxtune.fs.aygor

import android.net.Uri
import app.zxtune.fs.httpdir.PathBase

/*
 * 1) aygor:/${FilePath} - direct access to files or folders starting from http://abrimaal.pro-e.pl/ayon/ dir
 */
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris() = arrayOf(
        Uri.Builder()
            .scheme("http")
            .authority("abrimaal.pro-e.pl")
            .path("ayon/${getRemoteId()}")
            .build()
    )

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        if (elements.isEmpty()) EMPTY else Path(elements, isDir)

    companion object {

        private const val SCHEME = "aygor"

        private val EMPTY = Path(emptyList(), true)

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
