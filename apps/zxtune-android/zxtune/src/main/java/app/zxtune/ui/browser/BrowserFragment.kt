/**
 * @file
 * @brief Vfs browser fragment component
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.browser

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.support.v4.media.session.MediaControllerCompat
import android.view.*
import android.view.View.OnFocusChangeListener
import android.widget.ProgressBar
import android.widget.SearchView
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.selection.SelectionPredicates
import androidx.recyclerview.selection.SelectionTracker
import androidx.recyclerview.selection.StorageStrategy
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.OnChildAttachStateChangeListener
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.ui.utils.SelectionUtils

class BrowserFragment : Fragment() {
    private lateinit var model: Model
    private lateinit var stateStorage: State

    private var listing: RecyclerView? = null
    private var search: SearchView? = null

    private var selectionTracker: SelectionTracker<Uri>? = null

    private val controller
        get() = activity?.let { MediaControllerCompat.getMediaController(it) }
    private val listingAdapter
        get() = listing?.adapter as? ListingViewAdapter
    private val listingLayoutManager
        get() = listing?.layoutManager as? LinearLayoutManager

    private class NotificationView(view: View) {
        private val panel = view.findViewById<TextView>(R.id.browser_notification)

        fun bind(notification: Model.Notification?, onAction: (Intent) -> Unit) {
            setPanelVisibility(notification != null)
            panel.text = notification?.message
            panel.setOnClickListener(notification?.action?.let { intent ->
                View.OnClickListener { onAction(intent) }
            })
        }

        private fun setPanelVisibility(isVisible: Boolean) = panel.run {
            animate().translationY(if (isVisible) 0f else height.toFloat())
        }
    }

    override fun onAttach(ctx: Context) {
        super.onAttach(ctx)
        model = Model.of(this)
        stateStorage = State.create(ctx)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? = container?.let { inflater.inflate(R.layout.browser, it, false) }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        setupRootsView(view)
        setupBreadcrumbs(model, view)
        listing = setupListing(model, view)
        search = setupSearchView(model, view)
        setupProgress(model, view)
        setupNotification(model, view)
        if (model.state.value == null) {
            browse(stateStorage.currentPath)
        }
        model.setClient(object : Model.Client {
            override fun onFileBrowse(uri: Uri) =
                controller?.transportControls?.playFromUri(uri, null) ?: Unit

            override fun onError(msg: String) = showError(msg)
        })
    }

    private fun showError(msg: String) {
        view?.post {
            Toast.makeText(activity, msg, Toast.LENGTH_LONG).show()
        }
    }

    private fun setupRootsView(view: View) =
        view.findViewById<View>(R.id.browser_roots).setOnClickListener { browse(Uri.EMPTY) }

    private fun setupBreadcrumbs(model: Model, view: View) =
        view.findViewById<RecyclerView>(R.id.browser_breadcrumb).apply {
            val adapter = BreadcrumbsViewAdapter().apply {
                adapter = this
            }
            model.state.observe(viewLifecycleOwner) { state ->
                adapter.submitList(state.breadcrumbs)
                smoothScrollToPosition(state.breadcrumbs.size)
            }
            val onClick = View.OnClickListener { v: View ->
                val pos = getChildAdapterPosition(v)
                val uri = adapter.currentList[pos].uri
                browse(uri)
            }
            addOnChildAttachStateChangeListener(object : OnChildAttachStateChangeListener {
                override fun onChildViewAttachedToWindow(view: View) =
                    view.setOnClickListener(onClick)

                override fun onChildViewDetachedFromWindow(view: View) =
                    view.setOnClickListener(null)
            })
        }

    private fun setupListing(model: Model, view: View) =
        view.findViewById<RecyclerView>(R.id.browser_content).apply {
            setHasFixedSize(true)
            val adapter = ListingViewAdapter().apply {
                adapter = this
            }
            selectionTracker = SelectionTracker.Builder(
                "browser_selection",
                this, ListingViewAdapter.KeyProvider(adapter),
                ListingViewAdapter.DetailsLookup(this),
                StorageStrategy.createParcelableStorage(Uri::class.java)
            )
                .withSelectionPredicate(SelectionPredicates.createSelectAnything())
                .withOnItemActivatedListener { item, _ ->
                    item.selectionKey?.let { browse(it) }
                    true
                }.build().also {
                    adapter.setSelection(it.selection)
                    // another class for test
                    (activity as? AppCompatActivity)?.let { activity ->
                        SelectionUtils.install(activity, it, SelectionClient(adapter))
                    }
                }
            model.state.observe(viewLifecycleOwner) { state ->
                val stub = view.findViewById<TextView>(R.id.browser_stub)

                storeCurrentViewPosition()
                adapter.submitList(state.entries) {
                    stateStorage.currentPath = state.uri
                    restoreCurrentViewPosition()
                }
                if (state.entries.isEmpty()) {
                    visibility = View.GONE
                    stub.visibility = View.VISIBLE
                } else {
                    visibility = View.VISIBLE
                    stub.visibility = View.GONE
                }
            }
            lifecycle.addObserver(object : DefaultLifecycleObserver {
                override fun onStop(owner: LifecycleOwner) {
                    storeCurrentViewPosition()
                    lifecycle.removeObserver(this)
                }
            })
        }

    private fun setupProgress(model: Model, view: View) =
        view.findViewById<ProgressBar>(R.id.browser_loading).run {
            model.progress.observe(viewLifecycleOwner) { prg ->
                isIndeterminate = prg == -1
                progress = prg ?: 0
            }
        }

    private fun setupNotification(model: Model, view: View) = with(NotificationView(view)) {
        model.notification.observe(viewLifecycleOwner) {
            bind(it) { intent ->
                runCatching {
                    requireContext().startActivity(intent)
                }.onFailure { showError((it.cause ?: it).message ?: it.javaClass.name) }
            }
        }
    }

    private fun storeCurrentViewPosition() =
        listingLayoutManager?.findFirstVisibleItemPosition()
            ?.takeIf { it != RecyclerView.NO_POSITION }?.let { pos ->
                stateStorage.currentViewPosition = pos
            }

    private fun restoreCurrentViewPosition() =
        listingLayoutManager?.scrollToPositionWithOffset(
            stateStorage.currentViewPosition,
            0
        )

    private fun setupSearchView(model: Model, view: View) =
        view.findViewById<SearchView>(R.id.browser_search).apply {
            setOnSearchClickListener {
                layoutParams = layoutParams.apply {
                    width = ViewGroup.LayoutParams.MATCH_PARENT
                }
            }
            setOnQueryTextListener(object : SearchView.OnQueryTextListener {
                override fun onQueryTextSubmit(query: String): Boolean {
                    model.search(query)
                    clearFocus()
                    return true
                }

                override fun onQueryTextChange(query: String): Boolean {
                    listingAdapter?.setFilter(query)
                    return true
                }
            })
            setOnCloseListener {
                layoutParams = layoutParams.apply {
                    width = ViewGroup.LayoutParams.WRAP_CONTENT
                }
                post {
                    clearFocus()
                    model.reload()
                }
                false
            }
            onFocusChangeListener = OnFocusChangeListener { _, _ ->
                if (!isIconified && query.isEmpty()) {
                    isIconified = true
                }
            }
        }

    // ArchivesService for selection
    private inner class SelectionClient(private val adapter: ListingViewAdapter) :
        SelectionUtils.Client<Uri> {
        override fun getTitle(count: Int) = resources.getQuantityString(
            R.plurals.items,
            count, count
        )

        override fun getAllItems() = adapter.currentList.map { it.uri }

        override fun fillMenu(inflater: MenuInflater, menu: Menu) =
            inflater.inflate(R.menu.browser, menu)

        override fun processMenu(itemId: Int, selection: Selection<Uri>): Boolean {
            when (itemId) {
                R.id.action_add -> addToPlaylist(selection)
                else -> return false
            }
            return true
        }

        //TODO: rework for PlaylistControl usage as a local iface
        private fun addToPlaylist(selection: Selection<Uri>) = controller?.run {
            val params = Bundle().apply {
                putParcelableArrayList("uris", convertSelection(selection))
            }
            transportControls.sendCustomAction(MainService.CUSTOM_ACTION_ADD, params)
        }
    }

    override fun onSaveInstanceState(state: Bundle) {
        super.onSaveInstanceState(state)
        selectionTracker?.onSaveInstanceState(state)
        search?.takeIf { !it.isIconified }?.query?.let { query ->
            state.putString(SEARCH_QUERY_KEY, query.toString())
        }
    }

    override fun onViewStateRestored(state: Bundle?) {
        super.onViewStateRestored(state)
        selectionTracker?.onRestoreInstanceState(state)
        state?.getString(SEARCH_QUERY_KEY)?.takeIf { it.isNotEmpty() }?.let { query ->
            search?.run {
                post {
                    isIconified = false
                    setQuery(query, false)
                }
            }
        }
    }

    private fun browse(uri: Uri) = model.browse(uri)

    fun moveUp() =
        search?.takeIf { !it.isIconified }?.let { it.isIconified = true } ?: browseParent()

    private fun browseParent() = model.browseParent()

    companion object {
        private const val SEARCH_QUERY_KEY = "search_query"

        @JvmStatic
        fun createInstance() = BrowserFragment()

        private fun convertSelection(selection: Selection<Uri>) =
            ArrayList<Uri>(selection.size()).apply {
                selection.iterator().forEach(this::add)
            }
    }
}