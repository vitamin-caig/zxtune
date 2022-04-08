package app.zxtune.device

import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.os.storage.StorageManager
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService
import androidx.core.net.toUri
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.LiveData
import androidx.lifecycle.Transformations
import app.zxtune.Features
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.fs.VfsRootLocalStorageAccessFramework
import app.zxtune.fs.local.Identifier
import app.zxtune.fs.local.rootId
import app.zxtune.preferences.ProviderClient
import java.io.File

// Location uses platform-dependent uri format to store (treeUri for SAF and file scheme for legacy)
class PersistentStorage @VisibleForTesting constructor(private val ctx: Context) {
    interface State {
        val location: DocumentFile?

        // as required by system (e.g. document tree for  SAF)
        val defaultLocationHint: Uri
    }

    companion object {
        private const val PREFS_KEY = "persistent_storage"

        // TODO: localize?
        private const val DIR_NAME = "ZXTune"

        private val LOG = Logger(PersistentStorage::class.java.name)

        @JvmStatic
        val instance by lazy {
            PersistentStorage(MainApplication.getGlobalContext())
        }
    }

    private val client by lazy {
        ProviderClient(ctx)
    }

    val state: LiveData<State> by lazy {
        client.getLive(PREFS_KEY, "").let {
            Transformations.map(it) { path ->
                if (Features.StorageAccessFramework.isEnabled()) {
                    SAFState(ctx, path)
                } else {
                    LegacyState(path)
                }
            }
        }
    }

    fun setLocation(uri: Uri) {
        require(uri != Uri.EMPTY)
        if (Build.VERSION.SDK_INT >= 19) {
            Permission.requestPersistentStorageAccess(ctx, uri)
        }
        client.set(PREFS_KEY, uri.toString())
    }

    @RequiresApi(29)
    private class SAFState(ctx: Context, current: String) : State {
        override val location by lazy {
            current.takeIf { it.isNotEmpty() }?.let {
                runCatching { DocumentFile.fromTreeUri(ctx, Uri.parse(it)) }.getOrNull()
            }
        }
        override val defaultLocationHint by lazy {
            Identifier(
                requireNotNull(ctx.getSystemService<StorageManager>()).primaryStorageVolume.rootId(),
                DIR_NAME
            ).documentUri
        }
    }

    @Suppress("DEPRECATION")
    private class LegacyState(current: String) : State {
        override val location by lazy {
            val loc = if (current.isNotEmpty()) {
                File(current)
            } else {
                defaultLocation
            }.apply {
                val created = mkdirs()
                LOG.d { "Legacy dir at $this (created=$created)"}
            }
            DocumentFile.fromFile(loc)
        }
        override val defaultLocationHint
            get() = defaultLocation.toUri()

        private val defaultLocation by lazy {
            File(Environment.getExternalStorageDirectory(), DIR_NAME)
        }
    }
}
