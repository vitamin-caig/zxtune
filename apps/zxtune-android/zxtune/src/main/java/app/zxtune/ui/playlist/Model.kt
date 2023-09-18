package app.zxtune.ui.playlist

import android.app.Application
import android.database.Cursor
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.TimeStamp.Companion.fromMilliseconds
import app.zxtune.core.Identifier.Companion.parse
import app.zxtune.playlist.Database
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

typealias State = FilteredListState<Entry>

// public for provider
@OptIn(FlowPreview::class)
class Model @VisibleForTesting internal constructor(
    application: Application,
    private val client: ProviderClient,
    private val ioDispatcher: CoroutineDispatcher,
    defaultDispatcher: CoroutineDispatcher
) : AndroidViewModel(application) {

    private val _filter = MutableStateFlow("")
    private val _updates = callbackFlow {
        LOG.d { "Subscribe for changes " }
        client.registerObserver {
            trySend(Unit)
        }
        trySend(Unit)
        awaitClose {
            LOG.d { "Unsubscribe from changes" }
            client.unregisterObserver()
        }
    }.debounce(500)

    private val _stateFlow = merge(_updates, _filter).runningFold(createState()) { state, update ->
        if (update is String) {
            LOG.d { "Filtering with '$update'" }
            state.withFilter(update)
        } else {
            loadListing()?.let {
                LOG.d { "Updating with ${it.size} elements" }
                state.withContent(it)
            } ?: state
        }
    }.flowOn(defaultDispatcher)
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), createState())

    constructor(application: Application) : this(
        application, ProviderClient.create(application), Dispatchers.IO, Dispatchers.Default
    )

    val state
        get() = _stateFlow

    private suspend fun loadListing() = withContext(ioDispatcher) {
        client.query(null)?.use { cursor ->
            ArrayList<Entry>(cursor.count).apply {
                while (cursor.moveToNext()) {
                    add(createItem(cursor))
                }
            }
        }
    }

    var filter: String
        get() = _filter.value
        set(value) {
            _filter.value = value.trim()
        }

    fun sort(by: ProviderClient.SortBy, order: ProviderClient.SortOrder) =
        runAsync { client.sort(by, order) }

    fun move(id: Long, delta: Int) = runAsync { client.move(id, delta) }

    fun deleteAll() = runAsync { client.deleteAll() }

    fun delete(ids: LongArray) = runAsync {
        client.delete(ids)
    }

    private fun runAsync(task: () -> Unit) {
        viewModelScope.launch(ioDispatcher) {
            task()
        }
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)
        private fun createItem(cursor: Cursor) = Entry(
            cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal),
            parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal)),
            cursor.getString(Database.Tables.Playlist.Fields.title.ordinal),
            cursor.getString(Database.Tables.Playlist.Fields.author.ordinal),
            fromMilliseconds(cursor.getLong(Database.Tables.Playlist.Fields.duration.ordinal))
        )

        @VisibleForTesting
        fun createState() = State(::matchEntry)

        private fun matchEntry(entry: Entry, filter: String) =
            entry.title.contains(filter, true) || entry.author.contains(
                filter, true
            ) || entry.location.displayFilename.contains(filter, true)
    }
}
