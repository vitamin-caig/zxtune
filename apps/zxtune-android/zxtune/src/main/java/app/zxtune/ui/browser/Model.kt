package app.zxtune.ui.browser

import android.app.Application
import android.content.Intent
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.analytics.Analytics
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.fs.provider.VfsProviderClient.ParentsCallback
import app.zxtune.ui.browser.ListingEntry.Companion.makeFile
import app.zxtune.ui.browser.ListingEntry.Companion.makeFolder
import app.zxtune.ui.utils.FilteredListState
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
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
    private val async: ExecutorService,
) : AndroidViewModel(application) {

    interface Client {
        fun onFileBrowse(uri: Uri)
        fun onError(msg: String)
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

        fun withContent(
            breadcrumbs: List<BreadcrumbsEntry>,
            entries: List<ListingEntry>
        ) = State(breadcrumbs, listing.withContent(entries))

        fun withEntries(entries: List<ListingEntry>) =
            State(breadcrumbs, listing.withContent(entries))

        fun withFilter(filter: String) = State(breadcrumbs, listing.withFilter(filter))
    }

    data class Notification(
        val message: String,
        val action: Intent?,
    )

    @VisibleForTesting
    val mutableState = MutableLiveData<State>()
    private val mutableProgress = MutableLiveData<Int?>()
    private val mutableNotification = object : MutableLiveData<Notification?>() {
        private var uri: Uri? = null
        private var handle: Releaseable? = null

        fun release() = handle?.release() ?: Unit

        fun watch(uri: Uri) {
            // TODO: track onActive/onInactive
            this.uri = uri
            release()
            handle = providerClient.subscribeForNotifications(uri) {
                LOG.d { "Notification ${it?.message ?: "<none>"}" }
                postValue(it?.run { Notification(message, action) })
            }
        }
    }
    private val activeTaskSignal = AtomicReference<CancellationSignal>()
    private var client: Client = object : Client {
        override fun onFileBrowse(uri: Uri) = Unit
        override fun onError(msg: String) = Unit
    }

    @Suppress("UNUSED")
    constructor(application: Application) : this(
        application,
        VfsProviderClient(application),
        Executors.newSingleThreadExecutor()
    )

    override fun onCleared() = mutableNotification.release()

    val state: LiveData<State>
        get() = mutableState

    val progress: LiveData<Int?>
        get() = mutableProgress

    val notification: LiveData<Notification?>
        get() = mutableNotification

    fun setClient(client: Client) {
        this.client = client
    }

    fun browse(uri: Uri) {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE)
        executeAsync(BrowseTask(lazyOf(uri)))
    }

    fun browse(uri: Lazy<Uri>) = executeAsync(BrowseTask(uri))

    fun browseParent() = state.value?.takeIf { it.breadcrumbs.size > 1 }?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        executeAsync(BrowseParentTask(breadcrumbs))
    } ?: run {
        Analytics.sendBrowserEvent(Uri.EMPTY, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        browse(Uri.EMPTY)
    }

    fun reload() = state.value?.run {
        browse(uri)
    } ?: Unit

    fun search(query: String) = state.value?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_SEARCH)
        executeAsync(
            SearchTask(this, query)
        )
    } ?: Unit

    fun filter(rawFilter: String) = state.value?.run {
        async.execute {
            val newFilter = rawFilter.trim()
            if (filter != newFilter) {
                updateState {
                    withFilter(newFilter)
                }
            }
        }
    }

    private interface AsyncOperation {
        val description: String
        fun execute(signal: CancellationSignal)
    }

    private fun executeAsync(op: AsyncOperation) {
        val signal = CancellationSignal().apply {
            activeTaskSignal.getAndSet(this)?.cancel()
        }
        async.execute {
            LOG.d { "Started ${op.description}" }
            mutableProgress.postValue(-1)
            op.runCatching {
                execute(signal)
            }.onFailure {
                if (it is OperationCanceledException) {
                    LOG.d { "Canceled ${op.description}" }
                } else {
                    reportError(it)
                }
            }
            if (activeTaskSignal.compareAndSet(signal, null)) {
                mutableProgress.postValue(null)
                LOG.d { "Finished ${op.description}" }
            }
        }
    }

    private fun updateState(block: State.() -> State) = block(mutableState.value ?: State()).also {
        mutableState.postValue(it)
    }

    private fun commitState(
        breadcrumbs: List<BreadcrumbsEntry>,
        entries: List<ListingEntry>,
    ) {
        val newState = updateState {
            withContent(breadcrumbs, entries)
        }
        mutableNotification.watch(newState.uri)
    }

    private fun updateProgress(done: Int, total: Int) =
        mutableProgress.postValue(if (done == -1 || total == 0) done else 100 * done / total)

    private fun reportError(e: Throwable) = with((e.cause ?: e).message ?: e.javaClass.name) {
        client.onError(this)
    }

    private inner class BrowseTask(private val uriSource: Lazy<Uri>) : AsyncOperation {

        private val uri: Uri
            get() = uriSource.value

        override val description
            get() = "browse ${uri.takeUnless { it == Uri.EMPTY }?.toString() ?: "root"}"

        override fun execute(signal: CancellationSignal) = when (resolve(uri, signal)) {
            ObjectType.FILE, ObjectType.DIR_WITH_FEED -> client.onFileBrowse(uri)
            ObjectType.DIR -> commitState(getParents(), getEntries(uri, signal))
            else -> Unit
        }

        private fun getParents() = ArrayList<BreadcrumbsEntry>().apply {
            providerClient.parents(uri, object : ParentsCallback {
                override fun onObject(uri: Uri, name: String, icon: Int?) {
                    add(BreadcrumbsEntry(uri, name, icon))
                }

                override fun onProgress(done: Int, total: Int) = updateProgress(done, total)
            })
        }
    }

    private inner class BrowseParentTask(private val items: List<BreadcrumbsEntry>) :
        AsyncOperation {

        override val description
            get() = "browse parent"

        override fun execute(signal: CancellationSignal) {
            for (idx in items.size - 2 downTo 0) {
                if (tryBrowseAt(idx, signal)) {
                    break
                }
            }
        }

        private fun tryBrowseAt(idx: Int, signal: CancellationSignal): Boolean {
            val uri = items[idx].uri
            try {
                val type = resolve(uri, signal)
                if (ObjectType.DIR == type || ObjectType.DIR_WITH_FEED == type) {
                    commitState(items.subList(0, idx + 1), getEntries(uri, signal))
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

        override fun execute(signal: CancellationSignal) {
            publishState()
            // assume already resolved
            providerClient.search(uri, query, object : VfsProviderClient.ListingCallback {
                override fun onDir(
                    uri: Uri,
                    name: String,
                    description: String,
                    icon: Int?,
                    hasFeed: Boolean
                ) = Unit

                override fun onFile(
                    uri: Uri,
                    name: String,
                    description: String,
                    details: String,
                    tracks: Int?,
                    cached: Boolean?
                ) {
                    content.add(makeFile(uri, name, description, details, tracks, cached))
                    publishState()
                }

                override fun onProgress(done: Int, total: Int) = Unit
            }, signal)
        }

        private fun publishState() = updateState {
            withEntries(content)
        }
    }

    internal enum class ObjectType {
        DIR, DIR_WITH_FEED, FILE
    }

    private fun resolve(uri: Uri, signal: CancellationSignal): ObjectType? {
        var result: ObjectType? = null
        providerClient.resolve(uri, object : VfsProviderClient.ListingCallback {
            override fun onDir(
                uri: Uri, name: String, description: String, icon: Int?,
                hasFeed: Boolean
            ) {
                result = if (hasFeed) ObjectType.DIR_WITH_FEED else ObjectType.DIR
            }

            override fun onFile(
                uri: Uri, name: String, description: String, details: String, tracks: Int?,
                cached: Boolean?
            ) {
                result = ObjectType.FILE
            }

            override fun onProgress(done: Int, total: Int) = updateProgress(done, total)
        }, signal)
        return result
    }

    private fun getEntries(uri: Uri, signal: CancellationSignal) = ArrayList<ListingEntry>().apply {
        providerClient.list(uri, object : VfsProviderClient.ListingCallback {
            override fun onDir(
                uri: Uri,
                name: String,
                description: String,
                icon: Int?,
                hasFeed: Boolean
            ) {
                add(makeFolder(uri, name, description, icon))
            }

            override fun onFile(
                uri: Uri,
                name: String,
                description: String,
                details: String,
                tracks: Int?,
                cached: Boolean?
            ) {
                add(makeFile(uri, name, description, details, tracks, cached))
            }

            override fun onProgress(done: Int, total: Int) = updateProgress(done, total)
        }, signal)
    }

    companion object {
        private val LOG = Logger(Model::class.java.name)

        private fun matchEntry(entry: ListingEntry, filter: String) =
            entry.title.contains(filter, true) || entry.description.contains(filter, true)
    }
}
