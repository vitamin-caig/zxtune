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
import app.zxtune.Features
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.ResultActivity
import app.zxtune.fs.local.Identifier
import app.zxtune.fs.local.Utils.isMounted
import app.zxtune.fs.local.Utils.rootId
import app.zxtune.preferences.Preferences
import app.zxtune.preferences.ProviderClient
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.shareIn
import java.io.File

// Location uses platform-dependent uri format to store (treeUri for SAF and file scheme for legacy)
class PersistentStorage @VisibleForTesting constructor(
    private val ctx: Context, private val client: ProviderClient,
    private val dispatcher: CoroutineDispatcher,
) {
    interface State {
        val location: DocumentFile?

        // as required by system (e.g. document tree for  SAF)
        val defaultLocationHint: Uri
    }

    interface Subdirectory {
        fun tryGet(createIfAbsent: Boolean = false): DocumentFile?
    }

    companion object {
        private const val PREFS_KEY = "persistent_storage"

        // TODO: localize?
        private const val DIR_NAME = "ZXTune"

        private val LOG = Logger(PersistentStorage::class.java.name)

        @JvmStatic
        val instance by lazy {
            val ctx = MainApplication.getGlobalContext()
            PersistentStorage(ctx, Preferences.getProviderClient(ctx), Dispatchers.Main)
        }
    }

    private val scope = CoroutineScope(SupervisorJob() + dispatcher)

    val state by lazy {
        client.watchString(PREFS_KEY, "").map { path ->
            if (Features.StorageAccessFramework.isEnabled()) {
                SAFState(ctx, path)
            } else {
                LegacyState(path)
            }
        }.shareIn(scope, SharingStarted.Eagerly, replay = 1)
    }

    val setupIntent
        get() = state.map(this::toSetupIntent)

    private fun toSetupIntent(state: State) = if (true != state.location?.isDirectory) {
        ResultActivity.createPersistentStorageLocationRequestIntent(
            ctx, state.defaultLocationHint
        )
    } else {
        null
    }

    fun subdirectory(name: String): Subdirectory = object : Subdirectory {

        // TODO: use state.firstOrNull() after suspend tryGet or flowValueOf
        private val currentLocation
            get() = state.replayCache.lastOrNull()?.location

        override fun tryGet(createIfAbsent: Boolean) = currentLocation?.let { dir ->
            LOG.d { "Using persistent storage at ${dir.uri}" }
            val existing = dir.findFile(name)
            when {
                existing != null -> existing.takeIf { it.isDirectory }?.also {
                    LOG.d { "Reuse dir ${it.uri}" }
                }

                createIfAbsent -> dir.createDirectory(name)?.also {
                    LOG.d { "Create dir ${it.uri}" }
                }

                else -> null
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
        override val defaultLocationHint: Uri by lazy {
            val primaryStorage =
                requireNotNull(ctx.getSystemService<StorageManager>()).storageVolumes.firstOrNull {
                    it.isPrimary && it.isMounted()
                }
            primaryStorage?.run {
                Identifier(rootId(), DIR_NAME).documentUri
            } ?: Uri.EMPTY
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
                LOG.d { "Legacy dir at $this (created=$created)" }
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
