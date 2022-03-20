package app.zxtune.fs.scene

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.httpdir.PathBase

// not including /music element!!!

// music/{artists,compos,disks,groups/....
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris() = getRemoteId().let { path ->
        arrayOf(
            Cdn.scene(path),
            Uri.Builder()
                .scheme("https")
                .authority("archive.scene.org")
                .path("pub/music/${path}")
                .build()
        )
    }

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        if (elements.isEmpty()) EMPTY else Path(elements, isDir)

    companion object {

        private const val SCHEME = "scene"
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
