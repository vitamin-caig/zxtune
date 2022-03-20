package app.zxtune.fs.aminet

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.httpdir.PathBase

// not including /mods element!!!

// mods/${author}/${file}
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris() = getRemoteId().let { path ->
        arrayOf(
            Cdn.aminet(path),
            Uri.Builder()
                .scheme("http")
                .authority("aminet.net")
                .path("mods/${path}")
                .build()
        )
    }

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        if (elements.isEmpty()) EMPTY else Path(elements, isDir)

    companion object {

        private const val SCHEME = "aminet"

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
