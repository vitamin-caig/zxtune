package app.zxtune.fs.local

import android.content.Context
import android.os.Build
import android.os.storage.StorageManager
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService

internal class Api24StoragesSource
@VisibleForTesting constructor(
    private val ctx: Context,
    private val service: StorageManager
) : StoragesSource {

    @RequiresApi(24)
    override fun getStorages(visitor: StoragesSource.Visitor) =
        service.storageVolumes.forEach { volume ->
            volume.mountPoint()?.let { obj ->
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
