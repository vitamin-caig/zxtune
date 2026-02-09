package app.zxtune.ui.playlist

import android.app.Application
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.playlist.AggregatingProviderClient
import app.zxtune.playlist.Playlist
import app.zxtune.playlist.PlaylistContent
import app.zxtune.playlist.ProviderClient
import app.zxtune.playlist.Track
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.flow.shareIn
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

typealias State = FilteredListState<Track>

class Model @VisibleForTesting internal constructor(
    application: Application,
    private val client: AggregatingProviderClient,
    defaultDispatcher: CoroutineDispatcher,
) : AndroidViewModel(application) {
    private val defaultPlaylist = Playlist(Playlist.DEFAULT_ID, "Default")

    private val _currentPlaylistFlow = MutableStateFlow(defaultPlaylist)

    private val _allPlaylists = client.observePlaylists()
        .stateIn(viewModelScope, SHARING, arrayListOf(_currentPlaylistFlow.value))

    private val _playlistClient = _currentPlaylistFlow.map {
        client.getPlaylist(it.id)
    }.stateIn(viewModelScope, SHARING, client.getPlaylist(_currentPlaylistFlow.value.id))

    private val _filter = MutableSharedFlow<String>(
        replay = 0, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    @OptIn(ExperimentalCoroutinesApi::class)
    private val _updates
        get() = _playlistClient.flatMapLatest(ProviderClient::observeContent)

    private val _listingFlow = merge(_updates, _filter).runningFold(createState()) { state, update ->
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
    }.flowOn(defaultDispatcher).shareIn(viewModelScope, SHARING, 1)

    // public for provider
    constructor(application: Application) : this(
        application,
        AggregatingProviderClient.create(application),
        Dispatchers.Default,
    )

    val allPlaylists: ArrayList<Playlist>
        get() = _allPlaylists.value

    val playlist : Flow<Playlist>
        get() = _currentPlaylistFlow

    val controller
        get() = _playlistClient.value

    var currentPlaylist
        get() = _currentPlaylistFlow.value
        set(value) {
            _currentPlaylistFlow.tryEmit(value)
        }

    val listing: Flow<State>
        get() = _listingFlow

    var filter: String
        get() = _filter.replayCache.lastOrNull() ?: ""
        set(value) {
            _filter.tryEmit(value.trim())
        }

    fun scopeFor(tracks: Track.IdSet?) = Playlist.OperationScope(currentPlaylist.id, tracks)

    fun sort(spec: Playlist.Sorting) = runAsync { controller.sort(spec) }

    fun reorder(track: Track.Id, delta: Int) = runAsync { controller.reorder(track, delta) }

    fun deleteAll() = runAsync { controller.clear() }

    fun delete(tracks: Track.IdSet) = runAsync { controller.delete(tracks) }

    private fun runAsync(task: suspend () -> Unit) {
        viewModelScope.launch {
            task()
        }
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)
        private val SHARING = SharingStarted.WhileSubscribed(5000)

        @VisibleForTesting
        fun createState() = State(::matchEntry)

        private fun matchEntry(entry: Track, filter: String) =
            entry.meta.title.contains(filter, true) || entry.meta.author.contains(
                filter, true
            ) || entry.meta.location.displayFilename.contains(filter, true)
    }
}
