package app.zxtune.fs.cache

import java.io.File

interface CacheDir {
    fun find(vararg ids: String): File?
}
