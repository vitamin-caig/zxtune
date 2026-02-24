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
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.ProducerScope
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.channels.produce
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.collectIndexed
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

private val LOG = Logger(FolderQueue::class.java.name)

internal class FolderQueue(
    private val vfs: VfsProviderClient, val context: Uri, private val loader: Loader
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
                override fun onFile(file: Schema.Content.File) = file.toEntry()?.let {
                    add(it)
                    Unit
                } ?: Unit
            })
        }
    }

    private fun Schema.Content.File.toEntry() = when (type) {
        Schema.Content.File.Type.UNKNOWN -> Entry.Unknown(uri, indexation(uri))
        Schema.Content.File.Type.UNSUPPORTED -> null
        Schema.Content.File.Type.TRACK -> Entry.Track(Identifier(uri))
        Schema.Content.File.Type.REMOTE -> Entry.Unknown(uri, indexation(uri))
        Schema.Content.File.Type.ARCHIVE -> Entry.Archive(ArchiveScanner(scope, loader, uri))
    }

    private fun indexation(uri: Uri): Deferred<Entry?> = scope.async(start = CoroutineStart.LAZY) {
        LOG.d { "Indexing $uri" }
        var asFile: Schema.Content.File? = null
        var asDir: Schema.Content.Dir? = null
        vfs.resolve(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) {
                LOG.d { "Indexing $uri: $status" }
            }

            override fun onDir(dir: Schema.Content.Dir) {
                asDir = asDir ?: dir
            }

            override fun onFile(file: Schema.Content.File) {
                asFile = asFile ?: file
            }
        })
        asFile?.toEntry() ?: asDir?.let { Entry.Archive(ArchiveScanner(scope, loader, it.uri)) }
    }

    override val items: Flow<ReceiveChannel<PlayableItem>>
        get() = currentStream

    override suspend fun activate(uri: Uri) = state.navigate(uri)?.let {
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

    // TODO: think about playback history pre-fill from indexed archives.
    // Use SearchEngine with empty query to get all the files in the same order they
    // arrive while detection
    @JvmInline
    value class Archive(val scanner: ArchiveScanner) : Entry {
        override val uri
            get() = scanner.uri
    }
}

private typealias FolderStorage = ShuffledList<Entry>

private class FolderCursor(val file: ShuffledList.Cursor<Entry>, val trackIdx: Int = 0) {
    fun forTrack(idx: Int) = FolderCursor(file, idx)

    override fun toString() = "Folder$file@$trackIdx"
}

private class ArchiveScanner(
    private val scope: CoroutineScope, private val loader: Loader, val uri: Uri
) {
    val history = ArrayList<Identifier>()
    private val items = Channel<PlayableItem>(onUndeliveredElement = {
        it.module.release()
    }).apply {
        scope.launch {
            loader.detect(uri).collect(this@apply::send)
            close()
        }
    }

    @OptIn(DelicateCoroutinesApi::class)
    val detectionFinished
        get() = items.isClosedForReceive

    fun has(idx: Int) = idx >= 0 && (idx < history.size || !detectionFinished) // highly likely

    suspend fun total(): Int {
        if (!detectionFinished) {
            consumeFrom(history.size) { _, item ->
                item.module.release()
            }
        }
        return history.size
    }

    // TODO: share from FeedQueue
    suspend fun consumeFrom(start: Int, block: suspend (Int, PlayableItem) -> Unit) {
        if (start == 0 && history.isNotEmpty()) {
            return loader.detect(uri).collectIndexed(block)
        }
        for (cur in start..<history.size) {
            loader.load(history[cur])?.let { item ->
                block(cur, item)
            }
        }
        while (true) {
            items.receiveCatching().getOrNull()?.let {
                val index = history.size
                history.add(it.dataId)
                if (index >= start) {
                    block(index, it)
                } else {
                    it.module.release()
                }
            } ?: break
        }
    }
}

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

    suspend fun navigate(uri: Uri) = withContent {
        // Known limitation: when playback is stopped and resumed while playing some track
        // inside archive, next session's context will be that track parent dir, not an
        // original directory.
        find { uri == (it as? Entry.Track)?.id?.fullLocation }?.let {
            FolderCursor(it)
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
                advanceCursor(cur.file, +1)?.let {
                    FolderCursor(it)
                }
            } ?: break
        }
    }

    private suspend fun fetch(cur: FolderCursor) = withContent {
        getOrNull(cur.file)
    }

    private suspend fun resolveUnknown(cur: FolderCursor, entry: Entry) = withContent {
        replace(cur.file) { old ->
            if (old is Entry.Unknown) {
                LOG.d { "Resolve unknown at ${old.uri} to $entry" }
                entry
            } else {
                null
            }
        }
        Unit
    }

    private suspend fun ProducerScope<PlayableItem>.playEntry(
        cur: FolderCursor, entry: Entry
    ): Unit = when (entry) {
        is Entry.Track -> playTrack(cur, entry)
        is Entry.Archive -> playArchive(cur, entry)
        is Entry.Unknown -> if (entry.resolvedTo.isCompleted) {
            entry.resolvedTo.await()?.takeUnless { it is Entry.Unknown }?.let {
                resolveUnknown(cur, it)
                playEntry(cur, it)
            } ?: resolveUnknown(cur, Entry.Ignored)
        } else {
            playUri(cur, entry.uri)
        }

        else -> Unit
    }

    private suspend fun ProducerScope<PlayableItem>.playTrack(
        cur: FolderCursor, entry: Entry.Track
    ) = loader.load(entry.id)?.let {
        playItem(cur, it)
    } ?: Unit

    private suspend fun ProducerScope<PlayableItem>.playArchive(
        cur: FolderCursor, entry: Entry.Archive
    ) = entry.scanner.consumeFrom(cur.trackIdx) { idx, item ->
        playItem(cur.forTrack(idx), item)
    }

    private suspend fun ProducerScope<PlayableItem>.playUri(start: FolderCursor, uri: Uri) =
        with(ArchiveScanner(scope, loader, uri)) {
            LOG.d { "Play at $uri starting from $start" }
            consumeFrom(start.trackIdx) { idx, item ->
                playItem(start.forTrack(idx), item)
            }
            require(detectionFinished) { "Should be called on fully indexed archive" }
            when (history.size) {
                0 -> Entry.Ignored
                1 -> Entry.Track(history.first())
                else -> Entry.Archive(this@with)
            }.let {
                resolveUnknown(start, it)
            }
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
                    resolveUnknown(cur, replacement)
                    continue
                }

                is Entry.Archive -> {
                    val nextTrack = when {
                        initialStep -> cur.trackIdx + delta
                        delta < 0 -> element.scanner.total() + delta
                        else -> cur.trackIdx
                    }
                    if (element.scanner.has(nextTrack)) {
                        return cur.forTrack(nextTrack)
                    }
                }
            }
            cur = withContent {
                advanceCursor(cur.file, delta)?.let {
                    FolderCursor(it)
                }
            } ?: break
        }
        null
    }
}
