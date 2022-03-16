package app.zxtune.fs.local

import androidx.annotation.VisibleForTesting
import java.io.File
import java.io.FileInputStream
import java.io.InputStreamReader
import java.io.Reader

internal class LegacyStoragesSource
@VisibleForTesting constructor(private val delegate: Delegate) : StoragesSource {
    fun interface Delegate {
        fun getMounts(): Reader
    }

    override fun getStorages(visitor: StoragesSource.Visitor) = getStorages().forEach { obj ->
        visitor.onStorage(obj, "")
    }

    private fun getStorages() = getMounts().map { File(it) }.filter(Utils::isAcessibleStorage)
        .takeIf { it.isNotEmpty() } ?: File.listRoots().filter(Utils::isAcessibleStorage)

    private fun getMounts() = DirectoriesSet().apply {
        delegate.getMounts().use { reader ->
            reader.forEachLine { line ->
                //assume no comments in file
                line.split(' ').takeIf { it.size == 6 }?.run {
                    val device = get(0)
                    val mountpoint = get(1)
                    if ((device.startsWith(
                            "/dev/block/vold",
                            true
                        ) || device == "/dev/fuse") && !isBlacklisted(mountpoint)
                    ) {
                        insertElement(mountpoint)
                    }
                }
            }
        }
    }

    private fun isBlacklisted(mountpoint: String) =
        mountpoint == "/mnt/secure/asec" ||
                mountpoint.startsWith("/mnt/runtime/")

    private class DirectoriesSet : ArrayList<String>() {
        fun insertElement(newOne: String) {
            for (idx in indices) {
                val existing = get(idx)
                when {
                    newOne == existing || newOne.isSubdirOf(existing) -> return
                    existing.isSubdirOf(newOne) -> {
                        set(idx, newOne)
                        return
                    }
                }
            }
            add(newOne)
        }
    }

    companion object {
        private fun String.isSubdirOf(rh: String) =
            length > rh.length && startsWith(rh) && get(rh.length) == File.separatorChar

        fun create(): StoragesSource = LegacyStoragesSource(object : Delegate {
            override fun getMounts(): Reader =
                InputStreamReader(FileInputStream(File("/proc/mounts")))
        })
    }
}
