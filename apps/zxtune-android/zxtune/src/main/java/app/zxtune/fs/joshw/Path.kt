package app.zxtune.fs.joshw

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.httpdir.PathBase

/*
 * joshw:/${Catalogue}/${FilePath}
 */
class Path private constructor(elements: List<String>, isDir: Boolean) :
    PathBase(elements, isDir) {

    override fun getRemoteUris(): Array<Uri> {
        val catalogue = getCatalogue()
        val path = getInnerPath()
        return arrayOf(
            Cdn.joshw(catalogue, path),
            Uri.Builder()
                .scheme("http")
                .authority("${catalogue}.joshw.info")
                .path(path)
                .build()
        )
    }

    private fun getInnerPath(): String = getRemoteId().substringAfter('/')

    override fun getUri(): Uri = Uri.Builder().scheme(SCHEME).path(getRemoteId()).build()

    override fun build(elements: List<String>, isDir: Boolean) =
        // roots are always directories
        Path(elements, isDir || elements.size < 2)

    fun isCatalogue() = elements.size == 1

    fun getCatalogue() = elements.firstOrNull() ?: EMPTY_CATALOGUE

    companion object {

        private const val SCHEME = "joshw"
        private const val EMPTY_CATALOGUE = ""

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
