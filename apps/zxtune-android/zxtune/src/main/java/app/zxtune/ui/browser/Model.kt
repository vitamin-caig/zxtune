package app.zxtune.ui.browser

import android.app.Application
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import androidx.annotation.VisibleForTesting
import androidx.fragment.app.Fragment
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import app.zxtune.Logger
import app.zxtune.analytics.Analytics
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.fs.provider.VfsProviderClient.ParentsCallback
import app.zxtune.ui.browser.ListingEntry.Companion.makeFile
import app.zxtune.ui.browser.ListingEntry.Companion.makeFolder
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

/*
TODO:
- icons for files
- per-item progress and error status
- parallel browsing
*/
// public for provider
class Model @VisibleForTesting internal constructor(
    application: Application,
    private val providerClient: VfsProviderClient
) : AndroidViewModel(application) {

    interface Client {
        fun onFileBrowse(uri: Uri)
        fun onError(msg: String)
    }

    data class State(
        val uri: Uri,
        val breadcrumbs: List<BreadcrumbsEntry>,
        val entries: List<ListingEntry>,
        val query: String? = null,
    )

    private val async = Executors.newSingleThreadExecutor()

    @VisibleForTesting
    val mutableState = MutableLiveData<State>()
    private val mutableProgress = MutableLiveData<Int?>()
    private val activeTaskSignal = AtomicReference<CancellationSignal>()
    private var client: Client = object : Client {
        override fun onFileBrowse(uri: Uri) = Unit
        override fun onError(msg: String) = Unit
    }

    @Suppress("UNUSED")
    constructor(application: Application) : this(application, VfsProviderClient(application))

    val state: LiveData<State>
        get() = mutableState

    val progress: LiveData<Int?>
        get() = mutableProgress

    fun setClient(client: Client) {
        this.client = client
    }

    fun browse(uri: Uri) {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE)
        executeAsync(BrowseTask(uri), "browse $uri")
    }

    fun browseParent() = state.value?.takeIf { it.breadcrumbs.size > 1 }?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        executeAsync(BrowseParentTask(breadcrumbs), "browse parent")
    } ?: run {
        Analytics.sendBrowserEvent(Uri.EMPTY, Analytics.BROWSER_ACTION_BROWSE_PARENT)
        executeAsync(BrowseTask(Uri.EMPTY), "browse root")
    }

    fun reload() = state.value?.run {
        browse(uri)
    } ?: Unit

    fun search(query: String) = state.value?.run {
        Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_SEARCH)
        executeAsync(
            SearchTask(this, query), "search for $query in $uri"
        )
    } ?: Unit

    @VisibleForTesting
    fun waitForIdle() = with(ReentrantLock()) {
        val signal = newCondition()
        withLock {
            async.submit {
                withLock { signal.signalAll() }
            }
            signal.awaitUninterruptibly()
        }
    }

    private interface AsyncOperation {
        fun execute(signal: CancellationSignal)
    }

    private fun executeAsync(op: AsyncOperation, descr: String) {
        val signal = CancellationSignal().apply {
            activeTaskSignal.getAndSet(this)?.cancel()
        }
        async.execute {
            LOG.d { "Started $descr" }
            mutableProgress.postValue(-1)
            op.runCatching {
                execute(signal)
            }.onFailure {
                if (it is OperationCanceledException) {
                    LOG.d { "Canceled $descr" }
                } else {
                    reportError(it)
                }
            }
            if (activeTaskSignal.compareAndSet(signal, null)) {
                mutableProgress.postValue(null)
                LOG.d { "Finished $descr" }
            }
        }
    }

    private fun updateState(
        uri: Uri,
        breadcrumbs: List<BreadcrumbsEntry>,
        entries: List<ListingEntry>,
    ) = mutableState.postValue(State(uri, breadcrumbs, entries))

    private fun updateProgress(done: Int, total: Int) =
        mutableProgress.postValue(if (done == -1 || total == 0) done else 100 * done / total)

    private fun reportError(e: Throwable) = with((e.cause ?: e).message ?: e.javaClass.name) {
        client.onError(this)
    }

    private inner class BrowseTask(private val uri: Uri) : AsyncOperation {

        override fun execute(signal: CancellationSignal) = when (resolve(uri, signal)) {
            ObjectType.FILE, ObjectType.DIR_WITH_FEED -> client.onFileBrowse(uri)
            ObjectType.DIR -> updateState(uri, parents, getEntries(uri, signal))
            else -> Unit
        }

        private val parents: List<BreadcrumbsEntry>
            get() = ArrayList<BreadcrumbsEntry>().apply {
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
                    updateState(uri, items.subList(0, idx + 1), getEntries(uri, signal))
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
        private val content = mutableListOf<ListingEntry>()
        private val newState = State(state.uri, state.breadcrumbs, content, query)

        override fun execute(signal: CancellationSignal) {
            publishState()
            // assume already resolved
            providerClient.search(newState.uri, query, object : VfsProviderClient.ListingCallback {
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

        private fun publishState() = mutableState.postValue(newState)
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

        @JvmStatic
        fun of(owner: Fragment): Model = ViewModelProvider(
            owner,
            ViewModelProvider.AndroidViewModelFactory.getInstance(owner.requireActivity().application)
        )[Model::class.java]
    }
}
