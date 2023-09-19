package app.zxtune.ui.playlist

import android.app.Application
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
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
    private val _updates = client.observeChanges().debounce(500)

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

    val state: Flow<State>
        get() = _stateFlow

    private suspend fun loadListing() = withContext(ioDispatcher) {
        client.query(null)
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

        @VisibleForTesting
        fun createState() = State(::matchEntry)

        private fun matchEntry(entry: Entry, filter: String) =
            entry.title.contains(filter, true) || entry.author.contains(
                filter, true
            ) || entry.location.displayFilename.contains(filter, true)
    }
}
