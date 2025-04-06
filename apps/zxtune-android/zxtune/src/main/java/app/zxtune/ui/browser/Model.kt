package app.zxtune.ui.browser

import android.app.Application
import android.content.ContentResolver
import android.content.Intent
import android.net.Uri
import android.os.OperationCanceledException
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.analytics.Analytics
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.fs.provider.VfsProviderClient.ParentsCallback
import app.zxtune.ui.utils.FilteredListState
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.Job
import kotlinx.coroutines.async
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.receiveAsFlow
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

    private sealed interface Content
    private data class ListingContent(
        val breadcrumbs: List<BreadcrumbsEntry>,
        val entries: List<ListingEntry>,
    ) : Content {
        val uri: Uri
            get() = breadcrumbs.lastOrNull()?.uri ?: Uri.EMPTY
    }

    @JvmInline
    private value class FilterContent(val value: String) : Content

    @JvmInline
    private value class SearchingContent(val entries: List<ListingEntry>) : Content

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

        fun withContent(breadcrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>) =
            State(breadcrumbs, listing.withContent(entries))

        fun withEntries(entries: List<ListingEntry>) =
            State(breadcrumbs, listing.withContent(entries))

        fun withFilter(filter: String) = listing.withFilter(filter).let {
            if (listing === it) {
                this
            } else {
                State(breadcrumbs, it)
            }
        }
    }

    data class Notification(
        val message: String,
        val action: Intent?,
    )

    // Use SharedFlow(replay=1) to support initial value absence - no redundant data in _state
    // Current directory content snapshot. Use SharedFlow to support re-sending in cancelSearch
    private val _content =
        MutableSharedFlow<ListingContent>(replay = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    // Fast filter. Should be state to get scheduled value instead of delayed real from _state
    private val _filter =
        MutableSharedFlow<FilterContent>(replay = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    // Dynamic search content
    private val _searchContent = MutableSharedFlow<SearchingContent>(
        extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    // Mixin. Should be state in order to get latest content
    @OptIn(FlowPreview::class)
    private val _state = merge(
        _content, _filter.debounce(500), _searchContent
    ).runningFold(EMPTY_STATE) { state, update ->
        when (update) {
            is FilterContent -> {
                LOG.d { "Filtering with '${update.value}'" }
                state.withFilter(update.value)
            }

            is ListingContent -> state.withContent(update.breadcrumbs, update.entries)
            is SearchingContent -> state.withEntries(update.entries)
        }
    }.flowOn(defaultDispatcher).stateIn(
        viewModelScope, SharingStarted.WhileSubscribed(5000), EMPTY_STATE
    )

    // Notifications. Should be state to get latest value
    @OptIn(ExperimentalCoroutinesApi::class)
    private val _notifications =
        _content.map { it.uri }.distinctUntilChanged().flatMapLatest { uri ->
            providerClient.observeNotifications(uri).map {
                it?.run {
                    Notification(message, action)
                }
            }
        }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), null)

    // Operation progress. Should be state to get current value
    private val _progress = MutableStateFlow<Int?>(null)

    // Possible errors
    private val _errors = Channel<String>(capacity = Channel.CONFLATED) {
        LOG.d { "Ignored error '$it'" }
    }
    private val _toPlay = Channel<Uri>(capacity = Channel.CONFLATED) {
        LOG.d { "Ignored playback of $it" }
    }

    private val activeTask = AtomicReference<Job>()

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

    val errors
        get() = _errors.receiveAsFlow()

    val playbackEvents
        get() = _toPlay.receiveAsFlow()

    fun initialize(uri: Uri) {
        if (_state.value.uri == Uri.EMPTY) {
            executeAsync(BrowseTask(uri))
        }
    }

    fun browse(uri: Uri) {
        Analytics.sendBrowserEvent(uri, Analytics.BrowserAction.BROWSE)
        executeAsync(BrowseTask(uri))
    }

    fun browseParent() = _state.value.takeIf { it.breadcrumbs.size > 1 }?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BrowserAction.BROWSE_PARENT)
        executeAsync(BrowseParentTask(breadcrumbs))
    } ?: run {
        Analytics.sendBrowserEvent(Uri.EMPTY, Analytics.BrowserAction.BROWSE_PARENT)
        browse(Uri.EMPTY)
    }

    fun search(query: String) = _state.value.run {
        Analytics.sendBrowserEvent(uri, Analytics.BrowserAction.SEARCH)
        executeAsync(
            SearchTask(this, query)
        )
    }

    fun cancelSearch() {
        activeTask.getAndSet(null)?.cancel()
        _progress.value = null
        val content = _content.replayCache.lastOrNull() ?: ListingContent(emptyList(), emptyList())
        _content.tryEmit(content)
    }

    var filter: String
        get() = _filter.replayCache.lastOrNull()?.value ?: ""
        set(value) {
            _filter.tryEmit(FilterContent(value.trim()))
        }

    private interface AsyncOperation {
        val description: String
        suspend fun execute()
    }

    private val exceptionHandler = CoroutineExceptionHandler { ctx, e ->
        LOG.w(e) { "Failed ${ctx[CoroutineName]?.name}" }
        _progress.value = null
        if (e !is OperationCanceledException) {
            with((e.cause ?: e).message ?: e.javaClass.name) {
                _errors.trySend(this)
            }
        }
    }

    private fun executeAsync(op: AsyncOperation) {
        viewModelScope.launch {
            supervisorScope {
                launch(ioDispatcher + exceptionHandler + CoroutineName(op.description)) {
                    val job = coroutineContext.job.also {
                        activeTask.getAndSet(it)?.cancel()
                    }
                    LOG.d { "Started ${op.description}" }
                    _progress.value = -1
                    op.execute()
                    if (activeTask.compareAndSet(job, null)) {
                        _progress.value = null
                        LOG.d { "Finished ${op.description}" }
                    }
                }
            }
        }
    }

    private fun updateProgress(done: Int, total: Int) {
        _progress.value = if (done == -1 || total == 0) done else 100 * done / total
    }

    @VisibleForTesting
    suspend fun updateContent(
        breadcrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>
    ) = _content.emit(ListingContent(breadcrumbs, entries))

    private inner class BrowseTask(private val uri: Uri) : AsyncOperation {

        override val description
            get() = "browse ${uri.takeUnless { it == Uri.EMPTY }?.toString() ?: "root"}"

        override suspend fun execute() = when (resolve(uri)) {
            ObjectType.FILE, ObjectType.DIR_WITH_FEED -> coroutineScope { _toPlay.send(uri) }
            ObjectType.DIR -> coroutineScope {
                val entries = async { getEntries(uri) }
                val parents = getParents()
                updateContent(parents, entries.await())
            }

            else -> Unit
        }

        private suspend fun getParents() = ArrayList<BreadcrumbsEntry>().apply {
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

        override suspend fun execute() {
            for (idx in items.size - 2 downTo 0) {
                if (tryBrowseAt(idx)) {
                    break
                }
            }
        }

        private suspend fun tryBrowseAt(idx: Int): Boolean {
            val uri = items[idx].uri
            try {
                val type = resolve(uri)
                if (ObjectType.DIR == type || ObjectType.DIR_WITH_FEED == type) {
                    updateContent(items.subList(0, idx + 1), getEntries(uri))
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
        private val content = ArrayList<ListingEntry>(100)

        override val description
            get() = "search for $query at $uri"

        override suspend fun execute() = try {
            publishState()

            // assume already resolved
            providerClient.search(uri, query, object : VfsProviderClient.ListingCallback {
                override fun onProgress(status: Schema.Status.Progress) = Unit
                override fun onDir(dir: Schema.Listing.Dir) = Unit
                override fun onFile(file: Schema.Listing.File) {
                    content.add(makeFile(file))
                    publishState()
                }
            })
        } catch (e: Throwable) {
            cancelSearch()
            throw e
        }

        private fun publishState() {
            _searchContent.tryEmit(SearchingContent(content.toMutableList()))
        }
    }

    internal enum class ObjectType {
        DIR, DIR_WITH_FEED, FILE
    }

    private suspend fun resolve(uri: Uri): ObjectType? {
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
        })
        return result
    }

    private suspend fun getEntries(uri: Uri) = ArrayList<ListingEntry>().apply {
        providerClient.list(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) =
                updateProgress(status.done, status.total)

            override fun onDir(dir: Schema.Listing.Dir) {
                add(makeFolder(dir))
            }

            override fun onFile(file: Schema.Listing.File) {
                add(makeFile(file))
            }
        })
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)

        private val EMPTY_STATE = State()

        private fun matchEntry(entry: ListingEntry, filter: String) =
            entry.title.contains(filter, true) || entry.description.contains(filter, true)

        private fun makeFolder(dir: Schema.Listing.Dir) = dir.run {
            ListingEntry.makeFolder(uri, name, description, icon?.asListingEntryIcon())
        }

        private fun makeFile(file: Schema.Listing.File) = file.run {
            ListingEntry.makeFile(
                uri,
                name,
                description,
                details,
                type.asIcon(),
                icon?.asListingEntryIcon(),
            )
        }

        private fun Uri.asListingEntryIcon() = when (scheme) {
            ContentResolver.SCHEME_ANDROID_RESOURCE -> lastPathSegment?.toInt()?.let {
                ListingEntry.DrawableIcon(it)
            }

            else -> ListingEntry.LoadableIcon(this)
        }

        private fun Schema.Listing.File.Type.asIcon() = when (this) {
            Schema.Listing.File.Type.REMOTE -> R.drawable.ic_browser_file_remote
            Schema.Listing.File.Type.UNKNOWN -> R.drawable.ic_browser_file_unknown
            Schema.Listing.File.Type.TRACK -> R.drawable.ic_browser_file_track
            Schema.Listing.File.Type.ARCHIVE -> R.drawable.ic_browser_file_archive
            Schema.Listing.File.Type.UNSUPPORTED -> null
        }
    }
}
