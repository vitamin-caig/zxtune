package app.zxtune.playback.service

import android.content.Context
import android.net.Uri
import app.zxtune.Logger
import app.zxtune.playback.PlayableItem
import app.zxtune.playlist.PlaylistQuery
import app.zxtune.playlist.ProviderClient
import app.zxtune.playlist.Track
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.channels.produce
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

private val LOG = Logger(PlaylistQueue::class.java.name)

internal class PlaylistQueue(ctx: Context, loader: Loader) : Queue {
    private val scope = CoroutineScope(Dispatchers.IO)
    private val currentStream = MutableSharedFlow<ReceiveChannel<PlayableItem>>(
        replay = 1, extraBufferCapacity = 0, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    private val state = PlaylistQueueState(scope, loader, ProviderClient.create(ctx))

    override val items: Flow<ReceiveChannel<PlayableItem>>
        get() = currentStream

    override suspend fun activate(uri: Uri) {
        val id = Track.Id(requireNotNull(PlaylistQuery.idOf(uri)))
        state.navigate(id)?.let {
            activate(it)
        }
    }

    private suspend fun activate(cursor: PlaylistCursor) =
        currentStream.emit(state.channelFrom(cursor))

    override suspend fun next() = advance(+1)

    override suspend fun prev() = advance(-1)

    private suspend fun advance(delta: Int) = state.advancedPosition(delta)?.let {
        activate(it)
    } ?: Unit

    override var shuffled by state::shuffled

    override fun release() = scope.cancel()
}

private typealias PlaylistStorage = ShuffledList<Track>
private typealias PlaylistCursor = ShuffledList.Cursor<Track>

@OptIn(ExperimentalAtomicApi::class)
private class PlaylistQueueState(
    private val scope: CoroutineScope,
    private val loader: Loader,
    private val playlist: ProviderClient
) {
    private val lock = Mutex()
    private val content = PlaylistStorage { l, r ->
        l.id == r.id
    }
    private var contentChanged = true
    private val _currentPosition = AtomicReference<PlaylistCursor?>(null)
    var currentPosition
        get() = _currentPosition.load()
        private set(value) = _currentPosition.store(value)
    var shuffled by content::shuffled

    init {
        playlist.observeContent().onEach {
            LOG.d { "Invalidated content" }
            contentChanged = true
        }.launchIn(scope)
    }

    private suspend fun <T> withContent(block: PlaylistStorage.() -> T): T = lock.withLock {
        synchronizeContent()
        return content.block()
    }

    private suspend fun synchronizeContent() {
        if (contentChanged) {
            playlist.queryContent()?.let {
                LOG.d { "Updated content, ${it.size} items" }
                content.update(it)
                contentChanged = false
            }
        }
    }

    @OptIn(ExperimentalCoroutinesApi::class)
    fun channelFrom(first: PlaylistCursor) = scope.produce {
        LOG.d { "Start sequence from $first" }
        var current = first
        while (true) {
            loader.load(current.data)?.let {
                send(it)
                currentPosition = current
            }
            current = withContent { advanceCursor(current, +1) } ?: break
        }
    }

    suspend fun navigate(id: Track.Id) = withContent {
        find { it.id == id }
    }

    suspend fun advancedPosition(delta: Int) = currentPosition?.let { current ->
        withContent {
            advanceCursor(current, delta)
        }
    }
}