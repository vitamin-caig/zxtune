package app.zxtune.fs.local

import android.content.Context
import android.os.Build
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService
import java.io.File

internal class Api24StoragesSource
@VisibleForTesting constructor(
    private val ctx: Context,
    private val service: StorageManager
) : StoragesSource {

    @RequiresApi(24)
    private val getVolumeFile: (StorageVolume) -> File? = when {
        // already checked state
        Build.VERSION.SDK_INT >= 30 -> { vol: StorageVolume -> vol.directory }
        else -> object : (StorageVolume) -> File? {
            private val method = runCatching {
                StorageVolume::class.java.getMethod("getPathFile")
            }.getOrNull()

            override fun invoke(vol: StorageVolume) = if (Utils.isMountedStorage(vol.state)) {
                method?.invoke(vol) as? File
            } else {
                null
            }
        }
    }

    @RequiresApi(24)
    override fun getStorages(visitor: StoragesSource.Visitor) =
        service.storageVolumes.forEach { volume ->
            getVolumeFile(volume)?.let { obj ->
                visitor.onStorage(obj, volume.getDescription(ctx))
            }
        }

    companion object {
        fun maybeCreate(ctx: Context) =
            ctx.takeIf { Build.VERSION.SDK_INT >= 24 }?.getSystemService<StorageManager>()
                ?.let { service ->
                    Api24StoragesSource(ctx, service)
                }
    }
}
