package app.zxtune.playback.service

import android.net.Uri
import app.zxtune.Logger
import app.zxtune.core.Identifier
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.playback.PlayableItem
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.channels.produce
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

private val LOG = Logger(FeedQueue::class.java.name)

internal class FeedQueue(vfs: VfsProviderClient, val context: Uri, loader: Loader) : Queue {
    private val scope = CoroutineScope(Dispatchers.IO + CoroutineName("FeedQueue"))
    private val currentStream = MutableSharedFlow<ReceiveChannel<PlayableItem>>(
        replay = 1, extraBufferCapacity = 0, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    private val state = FeedQueueState(scope, loader, vfs.feed(context))

    override val items: Flow<ReceiveChannel<PlayableItem>>
        get() = currentStream

    override suspend fun activate(uri: Uri) {
        state.reset(uri.takeIf { it != context })
        activate(Cursor(0))
    }

    private suspend fun activate(pos: Cursor) = currentStream.emit(state.channelFrom(pos))

    override suspend fun next() = advance(+1)

    override suspend fun prev() = advance(-1)

    private suspend fun advance(delta: Int) = state.advancedPosition(delta)?.let {
        activate(it)
    } ?: Unit

    // No-op variable to keep track of shuffling state
    override var shuffled = false

    override fun release() = scope.cancel()
}

@JvmInline
private value class Cursor(val value: Int)

@OptIn(ExperimentalAtomicApi::class)
private class FeedQueueState(
    private val scope: CoroutineScope,
    private val loader: Loader,
    private val flow: Flow<Schema.Content.File>
) {
    private val history = ArrayList<Identifier>()
    private val _currentPosition = AtomicReference<Cursor?>(null)
    var currentPosition
        get() = _currentPosition.load()
        private set(value) = _currentPosition.store(value)

    fun reset(initial: Uri?) {
        history.clear()
        initial?.let {
            history.add(Identifier(it))
        }
    }

    @OptIn(ExperimentalCoroutinesApi::class)
    fun channelFrom(start: Cursor) = scope.produce {
        LOG.d { "Start sequence from $start" }
        currentPosition = null
        var cur = start.value
        while (cur < history.size) {
            val entry = history[cur]
            loader.load(entry)?.let {
                LOG.d { "History item $entry}" }
                send(it)
                currentPosition = Cursor(cur)
            }
            ++cur
        }
        flow.collect { file ->
            loader.detect(file.uri).collect {
                val index = history.size
                history.add(it.dataId)
                if (index >= cur) {
                    LOG.d { "Feed item ${it.dataId}" }
                    send(it)
                    currentPosition = Cursor(index)
                } else {
                    LOG.d { "Skip ${it.dataId}" }
                    it.module.release()
                }
            }
        }
    }

    fun advancedPosition(delta: Int) =
        currentPosition?.value?.plus(delta)?.takeIf { it >= 0 }?.let {
            Cursor(it)
        }
}
