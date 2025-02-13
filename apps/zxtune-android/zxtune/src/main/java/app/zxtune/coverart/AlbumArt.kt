package app.zxtune.coverart

import android.net.Uri
import androidx.core.text.isDigitsOnly
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
                pictureFilePriority(file.name)?.let { probability ->
                    if (probability > priority) {
                        priority = probability
                        candidate = file.uri
                    }
                }
            }
        })
        return candidate
    }

    fun pictureFilePriority(name: String): Int? {
        val delimiter = name.lastIndexOf('.')
        if (delimiter == -1) {
            return null
        }

        val ext = name.subSequence(delimiter + 1, name.length)
        val extPriority = matchPriority(ext, IMAGES_EXTENSIONS);
        if (0 == extPriority) {
            return null
        }

        val stem = name.subSequence(0, delimiter)
        val stemPriority = matchPriority(stem, COVER_ART_NAMES)
        if (0 != stemPriority) {
            return 100 * stemPriority + extPriority
        }
        if (isDCIM(stem)) {
            return 0
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
    private val DCIM_PREFIXES = arrayOf(
        "DSC_",
        "_DSC",
        "DSCN",
        "DSCF",
        "DSCF_",
        "DSC",
        "IMG_",
        "_MG_",
        "IMGP",
        "DJI_",
        "SAM_",
        "P",
        "L",
        "_JFK"
    )

    private fun isDCIM(stem: CharSequence) = DCIM_PREFIXES.find { prefix ->
        isDCIM(stem, prefix)
    } != null

    private fun isDCIM(stem: CharSequence, prefix: String) =
        stem.startsWith(prefix) && stem.substring(prefix.length).isDigitsOnly()
}
