package app.zxtune.fs.api

import android.net.Uri
import app.zxtune.BuildConfig

private val ROOT = Uri.parse(BuildConfig.CDN_ROOT)

object Cdn {
    fun asma(path: String): Uri = root.path("browse/asma/$path").build()

    fun hvsc(path: String): Uri = root.path("browse/hvsc/$path").build()

    fun joshw(catalogue: String, path: String): Uri =
        root.path("browse/joshw/$catalogue/$path").build()

    fun amp(id: Int): Uri = root.path("download/amp/ids/$id").build()

    // really url-encoded paths
    fun modland(id: String): Uri = root.encodedPath("download/modland$id").build()

    fun modarchive(id: Int): Uri = root.path("download/modarchive/ids/$id").build()

    fun aminet(path: String): Uri = root.path("download/aminet/mods/$path").build()

    fun vgmrips(path: String): Uri = root.path("download/vgmrips/$path").build()

    fun scene(path: String): Uri = root.path("browse/scene/music/$path").build()

    private val root: Uri.Builder
        get() = ROOT.buildUpon()
}
