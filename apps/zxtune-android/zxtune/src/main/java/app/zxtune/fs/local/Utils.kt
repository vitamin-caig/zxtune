package app.zxtune.fs.local

import android.net.Uri
import android.os.Build
import android.os.Environment
import android.os.storage.StorageVolume
import android.provider.DocumentsContract
import androidx.annotation.RequiresApi
import java.io.File

object Utils {
    internal fun isMountedStorage(state: String) =
        state == Environment.MEDIA_MOUNTED || state == Environment.MEDIA_MOUNTED_READ_ONLY

    @RequiresApi(24)
    private val storageGetPathFileMethod = runCatching {
        StorageVolume::class.java.getMethod("getPathFile")
    }.getOrNull()

    @JvmStatic // for shadowing
    fun isAcessibleStorage(obj: File) = obj.exists() && obj.isDirectory

    @RequiresApi(24)
    fun StorageVolume.isMounted() = isMountedStorage(state)

    @RequiresApi(24)
    fun StorageVolume.mountPoint() = when {
        Build.VERSION.SDK_INT >= 30 -> directory
        !isMounted() -> null
        else -> storageGetPathFileMethod?.invoke(this) as? File
    }

    @RequiresApi(29)
    fun StorageVolume.documentUri() = requireNotNull(
        createOpenDocumentTreeIntent()
            .getParcelableExtra<Uri>(DocumentsContract.EXTRA_INITIAL_URI)
    )

    @JvmStatic // for shadowing
    @RequiresApi(29)
    fun StorageVolume.rootId(): String = DocumentsContract.getRootId(documentUri())
}
