package app.zxtune.ui.playlist

import android.app.Application
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.playlist.Playlist
import app.zxtune.playlist.PlaylistContent
import app.zxtune.playlist.ProviderClient
import app.zxtune.playlist.Track
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.flow.shareIn
import kotlinx.coroutines.launch

typealias State = FilteredListState<Track>

class Model @VisibleForTesting internal constructor(
    application: Application,
    private val client: ProviderClient,
    defaultDispatcher: CoroutineDispatcher,
) : AndroidViewModel(application) {

    private val _filter = MutableSharedFlow<String>(
        replay = 0, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )
    private val _updates
        get() = client.observeContent()

    private val _stateFlow = merge(_updates, _filter).runningFold(createState()) { state, update ->
        when (update) {
            is String -> {
                LOG.d { "Filtering with '$update'" }
                state.withFilter(update)
            }

            is PlaylistContent -> {
                LOG.d { "Updating with ${update.size} elements" }
                state.withContent(update)
            }

            else -> state
        }
    }.flowOn(defaultDispatcher).shareIn(viewModelScope, SharingStarted.WhileSubscribed(5000), 1)

    // public for provider
    constructor(application: Application) : this(
        application,
        ProviderClient.create(application),
        Dispatchers.Default,
    )

    val state: Flow<State>
        get() = _stateFlow

    var filter: String
        get() = _filter.replayCache.lastOrNull() ?: ""
        set(value) {
            _filter.tryEmit(value.trim())
        }

    fun sort(spec: Playlist.Sorting) = runAsync { client.sort(spec) }

    fun reorder(track: Track.Id, delta: Int) = runAsync { client.reorder(track, delta) }

    fun deleteAll() = runAsync { client.deleteAll() }

    fun delete(tracks: Track.IdSet) = runAsync { client.delete(tracks) }

    private fun runAsync(task: suspend () -> Unit) {
        viewModelScope.launch {
            task()
        }
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)

        @VisibleForTesting
        fun createState() = State(::matchEntry)

        private fun matchEntry(entry: Track, filter: String) =
            entry.meta.title.contains(filter, true) || entry.meta.author.contains(
                filter, true
            ) || entry.meta.location.displayFilename.contains(filter, true)
    }
}
