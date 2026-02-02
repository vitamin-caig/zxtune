package app.zxtune.playback.service

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.core.Core
import app.zxtune.core.Identifier
import app.zxtune.core.Module
import app.zxtune.core.ModuleAttributes
import app.zxtune.core.ModuleDetectCallback
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsFile
import app.zxtune.playback.PlayableItem
import app.zxtune.playlist.PlaylistQuery
import app.zxtune.playlist.Track
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.trySendBlocking
import kotlinx.coroutines.flow.channelFlow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.withContext
import java.io.IOException

internal class Loader {
    suspend fun load(track: Track) = load(track.meta.location)?.let {
        object : PlayableItem by it {
            override val id
                get() = PlaylistQuery.uriFor(track.id.value)
            override val title
                get() = track.meta.title
            override val author
                get() = track.meta.author
        }
    }

    suspend fun load(id: Identifier) = runCatching {
        loadModule(id).asPlayableItem(id)
    }.onFailure { LOG.w(it) { "Failed to load $id" } }.getOrNull()

    // TODO: think about cleanup
    suspend fun load(uri: Uri) = Identifier(uri).let {
        loadModule(it).asPlayableItem(it)
    }

    fun detect(uri: Uri) = channelFlow {
        val file = resolve(uri)
        LOG.d { "detect()" }
        Core.detectModules(file, object : ModuleDetectCallback {
            override fun onModule(id: Identifier, obj: Module) {
                trySendBlocking(obj.asPlayableItem(id)).getOrThrow()
            }
        }, null)
    }.flowOn(Dispatchers.Default + CoroutineName("detectModules"))

    private suspend fun loadModule(id: Identifier) =
        loadModule(resolve(id.dataLocation), id.subPath)

    private suspend fun loadModule(file: VfsFile, subPath: String) =
        withContext(Dispatchers.Default + CoroutineName("loadModule")) {
            LOG.d { "loadModule($subPath)" }
            Core.loadModule(file, subPath)
        }

    private suspend fun resolve(uri: Uri) =
        withContext(Dispatchers.IO + CoroutineName("resolve")) {
            LOG.d { "resolve($uri)" }
            Vfs.resolve(uri) as? VfsFile ?: throw IOException("$uri is not a file")
        }

    private companion object {
        private val LOG = Logger(Loader::class.java.name)

        private fun Module.asPlayableItem(id: Identifier) = object : PlayableItem {
            override val module
                get() = this@asPlayableItem
            override val id
                get() = id.fullLocation
            override val dataId
                get() = id
            override val title
                get() = property(ModuleAttributes.TITLE)
            override val author
                get() = property(ModuleAttributes.AUTHOR)
            override val program
                get() = property(ModuleAttributes.PROGRAM)
            override val comment
                get() = property(ModuleAttributes.COMMENT)
            override val strings
                get() = property(ModuleAttributes.STRINGS)
            override val duration
                get() = module.duration
            override val size
                get() = getProperty(ModuleAttributes.SIZE, 0)

            private fun property(name: String) = getProperty(name, "")
        }
    }
}