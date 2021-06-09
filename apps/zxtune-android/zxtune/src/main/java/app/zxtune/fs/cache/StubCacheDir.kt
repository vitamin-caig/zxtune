package app.zxtune.fs.cache

import java.io.File

internal class StubCacheDir : CacheDir {
    override fun find(vararg ids: String): File? = null
}
