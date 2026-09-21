package app.zxtune.utils

import android.content.Context
import android.content.ContextWrapper
import app.zxtune.Logger
import java.io.File
import java.io.IOException
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

val Context.withCachedDirDatabases: Context
    get() = CachedDirDatabaseContext(this)

private class CachedDirDatabaseContext(base: Context) : ContextWrapper(base) {
    private val cachedDbs by lazy {
        File(super.cacheDir, "databases").apply {
            LOG.d { "Dbs at $this exists=$isDirectory" }
            if (!isDirectory && !mkdirs()) {
                LOG.w(IOException(absolutePath)) {
                    "Failed to create directory"
                }
            }
        }
    }
    private val lock = ReentrantReadWriteLock()

    override fun getDatabasePath(name: String): File = lock.read {
        val legacy = super.getDatabasePath(name)
        val modern = File(cachedDbs, name)
        if (legacy.isFile && (!modern.isFile || legacy.lastModified() > modern.lastModified())) {
            LOG.d {
                "Migrate ${legacy.absolutePath} -> ${modern.absolutePath} (${legacy.length()})"
            }
            lock.write {
                for (suffix in arrayOf("", "-shm", "-wal", "-journal")) {
                    val src = File(legacy.absolutePath + suffix)
                    if (!src.isFile) {
                        continue
                    }
                    val dst = File(modern.absolutePath + suffix).apply {
                        deleteRecursively()
                    }
                    LOG.d { "Rename $src -> $dst" }
                    if (!src.renameTo(dst)) {
                        LOG.d { " Failed to rename ${src.absolutePath}" }
                        if (suffix.isEmpty()) {
                            LOG.d { "Use legacy $src" }
                            return legacy
                        }
                        src.delete()
                    }
                }
            }
        }
        return modern
    }

    companion object {
        private val LOG = Logger("CachedDbs")
    }
}
