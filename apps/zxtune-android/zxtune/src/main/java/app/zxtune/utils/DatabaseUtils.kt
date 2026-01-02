package app.zxtune.utils

import android.content.Context
import android.content.ContextWrapper
import app.zxtune.Logger
import java.io.File
import java.io.IOException

val Context.withCachedDirDatabases: Context
    get() = CachedDirDatabaseContext(this)

private class CachedDirDatabaseContext(base: Context) : ContextWrapper(base) {
    private val cachedDbs by lazy {
        File(super.cacheDir, "databases").apply {
            if (!isDirectory && !mkdirs()) {
                LOG.w(IOException(absolutePath)) {
                    "Failed to create directory"
                }
            }
        }
    }

    override fun getDatabasePath(name: String): File {
        val legacy = super.getDatabasePath(name)
        val modern = File(cachedDbs, name)
        if (legacy.isFile && (!modern.isFile || legacy.lastModified() > modern.lastModified())) {
            LOG.d {
                "Migrate ${legacy.absolutePath} -> ${modern.absolutePath} (${legacy.length()})"
            }
            for (suffix in arrayOf("", "-shm", "-wal", "-journal")) {
                val src = File(legacy.absolutePath + suffix)
                if (!src.isFile) {
                    continue
                }
                val dst = File(modern.absolutePath + suffix).apply {
                    deleteRecursively()
                }
                if (!src.renameTo(dst)) {
                    LOG.d { " Failed to rename ${src.absolutePath}" }
                    if (suffix.isEmpty()) {
                        return legacy
                    }
                    src.delete()
                }
            }
        }
        return modern
    }

    companion object {
        private val LOG = Logger("CachedDbs")
    }
}
