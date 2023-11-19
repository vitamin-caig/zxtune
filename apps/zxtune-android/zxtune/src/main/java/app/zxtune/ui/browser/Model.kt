package app.zxtune.ui.browser

import android.app.Application
import android.content.Intent
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.analytics.Analytics
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.fs.provider.VfsProviderClient.ParentsCallback
import app.zxtune.ui.browser.ListingEntry.Companion.makeFile
import app.zxtune.ui.browser.ListingEntry.Companion.makeFolder
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.async
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.job
import kotlinx.coroutines.launch
import kotlinx.coroutines.supervisorScope
import java.util.concurrent.atomic.AtomicReference

/*
TODO:
- icons for files
- per-item progress and error status
- parallel browsing
*/
// public for provider
class Model @VisibleForTesting internal constructor(
    application: Application,
    private val providerClient: VfsProviderClient,
    private val ioDispatcher: CoroutineDispatcher,
    defaultDispatcher: CoroutineDispatcher,
) : AndroidViewModel(application) {

    data class Content(
        val breadcrumbs: List<BreadcrumbsEntry>,
        val entries: List<ListingEntry>,
    ) {
        val uri: Uri
            get() = breadcrumbs.lastOrNull()?.uri ?: Uri.EMPTY
    }

    class State private constructor(
        val breadcrumbs: List<BreadcrumbsEntry>,
        private val listing: FilteredListState<ListingEntry>
    ) {
        constructor() : this(emptyList(), FilteredListState(Companion::matchEntry))

        val uri: Uri
            get() = breadcrumbs.lastOrNull()?.uri ?: Uri.EMPTY

        val entries
            get() = listing.entries

        val filter
            get() = listing.filter

        fun withContent(content: Content) =
            State(content.breadcrumbs, listing.withContent(content.entries))

        fun withEntries(entries: List<ListingEntry>) =
            State(breadcrumbs, listing.withContent(entries))

        fun withFilter(filter: String) = State(breadcrumbs, listing.withFilter(filter))
    }

    data class Notification(
        val message: String,
        val action: Intent?,
    )

    // Current directory content snapshot. Use SharedFlow to support re-sending in cancelSearch
    private val _content =
        MutableSharedFlow<Content>(replay = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    // Fast filter. Should be state to get scheduled value instead of delayed real from _state
    private val _filter = MutableStateFlow("")

    // Dynamic search content
    private val _searchContent = MutableSharedFlow<List<ListingEntry>>(
        extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    // Mixin. Should be state in order to get latest content
    @OptIn(FlowPreview::class)
    private val _state = merge(
        _content, _filter.debounce(500), _searchContent
    ).runningFold(State()) { state, update ->
        when (update) {
            is String -> {
                LOG.d { "Filtering with '$update'" }
                state.withFilter(update)
            }

            is Content -> state.withContent(update)
            is List<*> -> state.withEntries(update.filterIsInstance<ListingEntry>())
            else -> state
        }
    }.flowOn(defaultDispatcher).stateIn(
        viewModelScope, SharingStarted.WhileSubscribed(5000), State()
    )

    // Notifications. Should be state to get latest value
    @OptIn(ExperimentalCoroutinesApi::class)
    private val _notifications = _content.flatMapLatest { content ->
        providerClient.observeNotifications(content.uri).map {
            it?.run {
                Notification(message, action)
            }
        }
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)

    // Operation progress. Should be state to get current value
    private val _progress = MutableStateFlow<Int?>(null)

    // Possible errors
    private val _errors = MutableSharedFlow<String>(
        extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )
    private val _playbackEvents = MutableSharedFlow<Uri>()

    private val activeTaskSignal = AtomicReference<CancellationSignal>()

    @Suppress("UNUSED")
    constructor(application: Application) : this(
        application, VfsProviderClient(application), Dispatchers.IO, Dispatchers.Default
    )

    val state: Flow<State>
        get() = _state

    val progress: Flow<Int?>
        get() = _progress

    val notification: Flow<Notification?>
        get() = _notifications

    val errors: Flow<String>
        get() = _errors

    val playbackEvents: Flow<Uri>
        get() = _playbackEvents

    fun initialize(uri: Lazy<Uri>) {
        if (_state.value.uri == Uri.EMPTY) {
            executeAsync(BrowseTask(uri))
        }
    }

    fun browse(uri: Uri) {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE)
        executeAsync(BrowseTask(lazyOf(uri)))
    }

    fun browseParent() = _state.value.takeIf { it.breadcrumbs.size > 1 }?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        executeAsync(BrowseParentTask(breadcrumbs))
    } ?: run {
        Analytics.sendBrowserEvent(Uri.EMPTY, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        browse(Uri.EMPTY)
    }

    fun search(query: String) = _state.value.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_SEARCH)
        executeAsync(
            SearchTask(this, query)
        )
    }

    fun cancelSearch() {
        activeTaskSignal.getAndSet(null)?.cancel()
        _progress.value = null
        val content = _content.replayCache.lastOrNull() ?: Content(emptyList(), emptyList())
        _content.tryEmit(content)
    }

    var filter: String
        get() = _filter.value
        set(value) {
            _filter.value = value.trim()
        }

    private interface AsyncOperation {
        val description: String
        suspend fun execute(signal: CancellationSignal)
    }

    private val exceptionHandler = CoroutineExceptionHandler { ctx, e ->
        if (e is OperationCanceledException) {
            ctx.job.cancel()
        } else {
            with((e.cause ?: e).message ?: e.javaClass.name) {
                _errors.tryEmit(this)
            }
        }
    }

    private fun executeAsync(op: AsyncOperation) {
        viewModelScope.launch {
            val signal = CancellationSignal().apply {
                activeTaskSignal.getAndSet(this)?.cancel()
            }
            supervisorScope {
                val job = launch(ioDispatcher + exceptionHandler) {
                    LOG.d { "Started ${op.description}" }
                    _progress.value = -1
                    op.execute(signal)
                    if (activeTaskSignal.compareAndSet(signal, null)) {
                        _progress.value = null
                        LOG.d { "Finished ${op.description}" }
                    }
                }
                signal.setOnCancelListener {
                    job.cancel()
                }
            }
        }
    }

    private fun updateProgress(done: Int, total: Int) {
        _progress.value = if (done == -1 || total == 0) done else 100 * done / total
    }

    private suspend fun updateContent(
        breadcrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>
    ) = _content.emit(Content(breadcrumbs, entries))

    private inner class BrowseTask(private val uriSource: Lazy<Uri>) : AsyncOperation {

        private val uri: Uri
            get() = uriSource.value

        override val description
            get() = "browse ${uri.takeUnless { it == Uri.EMPTY }?.toString() ?: "root"}"

        override suspend fun execute(signal: CancellationSignal) = when (resolve(uri, signal)) {
            ObjectType.FILE, ObjectType.DIR_WITH_FEED -> _playbackEvents.emit(uri)
            ObjectType.DIR -> coroutineScope {
                val entries = async { getEntries(uri, signal) }
                val parents = getParents()
                updateContent(parents, entries.await())
            }

            else -> Unit
        }

        private fun getParents() = ArrayList<BreadcrumbsEntry>().apply {
            providerClient.parents(uri, object : ParentsCallback {
                override fun onObject(obj: Schema.Parents.Object) {
                    add(BreadcrumbsEntry(obj.uri, obj.name, obj.icon))
                }
            })
        }
    }

    private inner class BrowseParentTask(private val items: List<BreadcrumbsEntry>) :
        AsyncOperation {

        override val description
            get() = "browse parent"

        override suspend fun execute(signal: CancellationSignal) {
            for (idx in items.size - 2 downTo 0) {
                if (tryBrowseAt(idx, signal)) {
                    break
                }
            }
        }

        private suspend fun tryBrowseAt(idx: Int, signal: CancellationSignal): Boolean {
            val uri = items[idx].uri
            try {
                val type = resolve(uri, signal)
                if (ObjectType.DIR == type || ObjectType.DIR_WITH_FEED == type) {
                    updateContent(items.subList(0, idx + 1), getEntries(uri, signal))
                    return true
                }
            } catch (e: OperationCanceledException) {
                throw e
            } catch (e: Exception) {
                LOG.d { "Skipping $uri while navigating up" }
            }
            return false
        }
    }

    private inner class SearchTask(state: State, private val query: String) : AsyncOperation {
        private val uri = state.uri
        private val content = mutableListOf<ListingEntry>()

        override val description
            get() = "search for $query at $uri"

        override suspend fun execute(signal: CancellationSignal) {
            publishState()

            // assume already resolved
            providerClient.search(uri, query, object : VfsProviderClient.ListingCallback {
                override fun onProgress(status: Schema.Status.Progress) = Unit
                override fun onDir(dir: Schema.Listing.Dir) = Unit
                override fun onFile(file: Schema.Listing.File) {
                    content.add(makeFile(file))
                    if (!signal.isCanceled) {
                        publishState()
                    }
                }
            }, signal)
        }

        private fun publishState() {
            _searchContent.tryEmit(content)
        }
    }

    internal enum class ObjectType {
        DIR, DIR_WITH_FEED, FILE
    }

    private fun resolve(uri: Uri, signal: CancellationSignal): ObjectType? {
        var result: ObjectType? = null
        providerClient.resolve(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) =
                updateProgress(status.done, status.total)

            override fun onDir(dir: Schema.Listing.Dir) {
                result = if (dir.hasFeed) ObjectType.DIR_WITH_FEED else ObjectType.DIR
            }

            override fun onFile(file: Schema.Listing.File) {
                result = ObjectType.FILE
            }
        }, signal)
        return result
    }

    private fun getEntries(uri: Uri, signal: CancellationSignal) = ArrayList<ListingEntry>().apply {
        providerClient.list(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) =
                updateProgress(status.done, status.total)

            override fun onDir(dir: Schema.Listing.Dir) {
                add(makeFolder(dir))
            }

            override fun onFile(file: Schema.Listing.File) {
                add(makeFile(file))
            }
        }, signal)
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)

        private fun matchEntry(entry: ListingEntry, filter: String) =
            entry.title.contains(filter, true) || entry.description.contains(filter, true)

        private fun makeFolder(dir: Schema.Listing.Dir) = dir.run {
            makeFolder(uri, name, description, icon)
        }

        private fun makeFile(file: Schema.Listing.File) = file.run {
            makeFile(uri, name, description, details, tracks, isCached)
        }
    }
}
