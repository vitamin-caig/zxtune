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

    @JvmStatic // for shadowing
    fun isAcessibleStorage(obj: File) = obj.exists() && obj.isDirectory

    @RequiresApi(24)
    internal val storageGetPathFileMethod = runCatching {
        StorageVolume::class.java.getMethod("getPathFile")
    }.getOrNull()
}

@RequiresApi(24)
fun StorageVolume.isMounted() = Utils.isMountedStorage(state)

@RequiresApi(24)
fun StorageVolume.mountPoint() = when {
    Build.VERSION.SDK_INT >= 30 -> directory
    !isMounted() -> null
    else -> Utils.storageGetPathFileMethod?.invoke(this) as? File
}

@RequiresApi(29)
fun StorageVolume.documentUri() = requireNotNull(
    createOpenDocumentTreeIntent()
        .getParcelableExtra<Uri>(DocumentsContract.EXTRA_INITIAL_URI)
)

@RequiresApi(29)
fun StorageVolume.rootId(): String = DocumentsContract.getRootId(documentUri())
