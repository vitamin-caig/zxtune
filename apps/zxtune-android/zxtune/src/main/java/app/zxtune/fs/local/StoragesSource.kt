package app.zxtune.fs.local

import android.content.Context
import app.zxtune.Features
import java.io.File

interface StoragesSource {
    fun interface Visitor {
        fun onStorage(obj: File, description: String)
    }

    fun getStorages(visitor: Visitor)

    companion object {
        @JvmStatic
        fun create(ctx: Context) = object : StoragesSource {
            private val delegate = if (Features.StorageManagerVolumes.isEnabled()) {
                Api24StoragesSource.create(ctx)
            } else {
                LegacyStoragesSource.create()
            }

            override fun getStorages(visitor: Visitor) {
                delegate.getStorages(visitor)
                StandardStoragesSource.getStorages(visitor)
            }
        }
    }
}
