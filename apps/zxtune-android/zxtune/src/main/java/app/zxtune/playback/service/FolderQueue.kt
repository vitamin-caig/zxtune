package app.zxtune.playback.service

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.core.Identifier
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.playback.PlayableItem
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.ProducerScope
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.channels.produce
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

private val LOG = Logger(FolderQueue::class.java.name)

internal class FolderQueue(
    private val vfs: VfsProviderClient, val context: Uri, loader: Loader
) : Queue {
    private val scope = CoroutineScope(Dispatchers.IO + CoroutineName("FolderQueue"))
    private val currentStream = MutableSharedFlow<ReceiveChannel<PlayableItem>>(
        replay = 1, extraBufferCapacity = 0, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    private val state = FolderQueueState(scope, loader) {
        ArrayList<Entry>().apply {
            vfs.list(context, object : VfsProviderClient.ListingCallback {
                override fun onProgress(status: Schema.Status.Progress) = Unit
                override fun onDir(dir: Schema.Content.Dir) = Unit
                override fun onFile(file: Schema.Content.File) {
                    file.toEntry()?.let {
                        add(it)
                    }
                }
            })
        }
    }

    private fun Schema.Content.File.toEntry() = when (type) {
        Schema.Content.File.Type.UNKNOWN -> Entry.Unknown(uri, indexation(uri))
        Schema.Content.File.Type.UNSUPPORTED -> null
        Schema.Content.File.Type.TRACK -> Entry.Track(Identifier(uri))
        Schema.Content.File.Type.REMOTE -> Entry.Unknown(uri, indexation(uri))
        Schema.Content.File.Type.ARCHIVE -> null // treat archives as a folders, so ignore
    }

    private fun indexation(uri: Uri): Deferred<Entry?> = scope.async(start = CoroutineStart.LAZY) {
        LOG.d { "Indexing $uri" }
        var asFile: Schema.Content.File? = null
        vfs.resolve(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) {
                LOG.d { "Indexing $uri: $status" }
            }

            override fun onDir(dir: Schema.Content.Dir) = Unit

            override fun onFile(file: Schema.Content.File) {
                asFile = asFile ?: file
            }
        })
        asFile?.toEntry()
    }

    override val items: Flow<ReceiveChannel<PlayableItem>>
        get() = currentStream

    override suspend fun activate(uri: Uri) = state.find(uri)?.let {
        activate(it)
    } ?: Unit

    private suspend fun activate(cursor: FolderCursor) =
        currentStream.emit(state.channelFrom(cursor))

    override suspend fun next() = advance(+1)

    override suspend fun prev() = advance(-1)

    private suspend fun advance(delta: Int) = state.advancedPosition(delta)?.let {
        activate(it)
    } ?: Unit

    override var shuffled by state::shuffled

    override fun release() = scope.cancel()
}

private sealed interface Entry {
    val uri: Uri?

    object Ignored : Entry {
        override val uri
            get() = null
    }

    class Unknown(override val uri: Uri, val resolvedTo: Deferred<Entry?>) : Entry

    @JvmInline
    value class Track(val id: Identifier) : Entry {
        override val uri
            get() = id.fullLocation
    }
}

private typealias FolderStorage = ShuffledList<Entry>
private typealias FolderCursor = ShuffledList.Cursor<Entry>

@OptIn(ExperimentalAtomicApi::class, ExperimentalCoroutinesApi::class)
private class FolderQueueState(
    private val scope: CoroutineScope,
    private val loader: Loader,
    private val createContent: suspend () -> ArrayList<Entry>
) {
    private val lock = Mutex()
    private val content = FolderStorage { l, h ->
        l === h || true == l.uri?.let { it == h.uri }
    }
    private val _currentPosition = AtomicReference<FolderCursor?>(null)
    var currentPosition
        get() = _currentPosition.load()
        private set(value) = _currentPosition.store(value)

    var shuffled by content::shuffled

    private suspend fun <T> withContent(block: FolderStorage.() -> T): T = lock.withLock {
        if (0 == content.size) {
            content.update(createContent())
            LOG.d { "Loaded folder content, ${content.size} entries" }
        }
        return content.block()
    }

    suspend fun find(uri: Uri) = withContent {
        find {
            (it as? Entry.Track)?.id?.run {
                uri == fullLocation || uri == dataLocation
            } ?: false
        }
    }

    fun channelFrom(start: FolderCursor) = scope.produce {
        LOG.d { "Start sequence from $start" }
        currentPosition = null
        var cur = start
        while (true) {
            val toPlay = fetch(cur) ?: break
            playEntry(cur, toPlay)
            cur = withContent {
                advanceCursor(cur, +1)
            } ?: break
        }
    }

    private suspend fun fetch(cur: FolderCursor) = withContent {
        getOrNull(cur)
    }

    private suspend fun replace(cur: FolderCursor, replacement: Entry) = withContent {
        replace(cur) { old ->
            when (old) {
                is Entry.Unknown -> {
                    LOG.d { "Resolve unknown at ${old.uri} to $replacement" }
                    replacement
                }

                is Entry.Track if replacement is Entry.Track -> {
                    LOG.d { "Clarify ${old.uri} to ${replacement.uri}" }
                    replacement
                }

                else -> null
            }
        }
        Unit
    }

    private suspend fun ProducerScope<PlayableItem>.playEntry(
        cur: FolderCursor, entry: Entry
    ): Unit = when (entry) {
        is Entry.Track -> playTrack(cur, entry)
        is Entry.Unknown -> entry.resolvedTo.await()?.takeUnless { it is Entry.Unknown }?.let {
            replace(cur, it)
            playEntry(cur, it)
        } ?: replace(cur, Entry.Ignored)

        else -> Unit
    }

    private suspend fun ProducerScope<PlayableItem>.playTrack(
        cur: FolderCursor, entry: Entry.Track
    ) = if (entry.id.subPath.isEmpty()) {
        // Track may be single in archive, so resolve it again
        loader.detect(entry.uri).collect {
            if (it.dataId.subPath.isNotEmpty()) {
                replace(cur, Entry.Track(it.dataId))
            }
            playItem(cur, it)
        }
    } else {
        loader.load(entry.id)?.let {
            playItem(cur, it)
        } ?: Unit
    }

    private suspend fun ProducerScope<PlayableItem>.playItem(
        cursor: FolderCursor, item: PlayableItem
    ) {
        send(item)
        currentPosition = cursor
    }

    suspend fun advancedPosition(delta: Int): FolderCursor? = currentPosition?.let { start ->
        var cur = start
        while (true) {
            val initialStep = cur === start
            when (val element = fetch(cur)) {
                null -> break
                is Entry.Track -> if (!initialStep) {
                    return cur
                }

                is Entry.Ignored -> Unit
                is Entry.Unknown -> {
                    val replacement = element.resolvedTo.await()?.takeUnless { it is Entry.Unknown }
                        ?: Entry.Ignored
                    replace(cur, replacement)
                    continue
                }
            }
            cur = withContent {
                advanceCursor(cur, delta)
            } ?: break
        }
        null
    }
}
