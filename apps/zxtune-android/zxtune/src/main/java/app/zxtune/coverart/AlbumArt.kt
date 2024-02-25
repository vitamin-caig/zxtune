package app.zxtune.coverart

import android.net.Uri
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile

object AlbumArt {
    @JvmStatic
    fun forDir(obj: VfsDir): Uri? {
        var candidate: Uri? = null
        var priority = 0
        obj.enumerate(object : VfsDir.Visitor() {
            override fun onDir(dir: VfsDir) = Unit
            override fun onFile(file: VfsFile) {
                val probability = pictureFilePriority(file.name)
                if (probability > priority) {
                    priority = probability
                    candidate = file.uri
                }
            }
        })
        return candidate
    }

    fun pictureFilePriority(name: String): Int {
        val delimiter = name.lastIndexOf('.')
        if (delimiter == -1) {
            return 0;
        }

        val ext = name.subSequence(delimiter + 1, name.length)
        val extPriority = matchPriority(ext, IMAGES_EXTENSIONS);
        if (0 == extPriority) {
            return 0;
        }

        val stem = name.subSequence(0, delimiter)
        val stemPriority = matchPriority(stem, COVER_ART_NAMES)
        if (0 != stemPriority) {
            return 100 * stemPriority + extPriority
        }
        val stemFuzzyPriority =
            1 + COVER_ART_NAMES.indexOfLast { stem.contains(it, ignoreCase = true) }
        return 10 * stemFuzzyPriority + extPriority
    }

    private fun matchPriority(str: CharSequence, elements: Array<String>) =
        1 + elements.indexOfFirst { it.contentEquals(str, ignoreCase = true) }

    // from lower to higher priority
    private val IMAGES_EXTENSIONS = arrayOf("png", "jpeg", "jpg")
    private val COVER_ART_NAMES = arrayOf("back", "title", "front", "folder", "cover")
}
