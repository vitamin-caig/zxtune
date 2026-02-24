package app.zxtune.playback.service

import android.net.Uri
import app.zxtune.Releaseable
import app.zxtune.playback.PlayableItem
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.emptyFlow

// Input cases:
// - playlist: bidirectional sequence, context is playlist uri, looping/shuffling
// - saved playlist: bidirectional sequence, context is file uri, looping/shuffling
// - folder: bidirectional delayloaded sequence, context is folder uri, ?/?
// - stream: unidirectional (+history), context is folder uri, -/-
// - search result: bidirectional delayloaded sequence, context is vfs provider query uri, ?/?

interface Queue : Releaseable {
    val items: Flow<ReceiveChannel<PlayableItem>>

    suspend fun activate(uri: Uri)

    suspend fun next()
    suspend fun prev()

    var shuffled: Boolean
}

object EmptyQueue : Queue {
    override val items
        get() = emptyFlow<ReceiveChannel<PlayableItem>>()

    override suspend fun activate(uri: Uri) = Unit
    override suspend fun next() = Unit
    override suspend fun prev() = Unit

    override var shuffled
        get() = false
        set(_) = Unit

    override fun release() = Unit
}

