package app.zxtune.fs.local

import android.content.Context
import android.os.storage.StorageManager
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService
import app.zxtune.fs.local.Utils.mountPoint

@RequiresApi(24)
internal class Api24StoragesSource
@VisibleForTesting constructor(
    private val ctx: Context,
    private val service: StorageManager
) : StoragesSource {

    override fun getStorages(visitor: StoragesSource.Visitor) =
        service.storageVolumes.forEach { volume ->
            volume.mountPoint()?.let { obj ->
                visitor.onStorage(obj, volume.getDescription(ctx))
            }
        }

    companion object {
        fun create(ctx: Context) = Api24StoragesSource(ctx, requireNotNull(ctx.getSystemService()))
    }
}
