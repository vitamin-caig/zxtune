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
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.View
import android.view.View.OnFocusChangeListener
import android.view.ViewGroup
import android.widget.ProgressBar
import android.widget.SearchView
import android.widget.TextView
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
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
import app.zxtune.MainActivity
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.ui.utils.SelectionUtils
import app.zxtune.utils.ifNotNulls

class BrowserFragment : Fragment(), MainActivity.PagerTabListener {
    private lateinit var model: Model
    private val stateStorage by lazy {
        State.create(requireContext())
    }

    private val backHandler by lazy {
        object : OnBackPressedCallback(false) {
            override fun handleOnBackPressed() = moveUp()
        }.also {
            requireActivity().onBackPressedDispatcher.addCallback(this, it)
        }
    }

    private var listing: RecyclerView? = null
    private var search: SearchView? = null

    private var selectionTracker: SelectionTracker<Uri>? = null

    private val controller
        get() = activity?.let { MediaControllerCompat.getMediaController(it) }
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

    override fun onTabVisibilityChanged(isVisible: Boolean) {
        backHandler.isEnabled = isVisible
    }

    override fun onAttach(ctx: Context) {
        super.onAttach(ctx)
        model = Model.of(this)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
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
            model.browse(lazy {
                stateStorage.currentPath
            })
        }
        model.setClient(object : Model.Client {
            override fun onFileBrowse(uri: Uri) =
                controller?.transportControls?.playFromUri(uri, null) ?: Unit

            override fun onError(msg: String) = showError(msg)
        })
    }

    private fun showError(msg: String) {
        ifNotNulls(view, activity) { view, activity ->
            view.post {
                Toast.makeText(activity, msg, Toast.LENGTH_LONG).show()
            }
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
                if (!isInSearchMode()) {
                    adapter.submitList(state.breadcrumbs)
                    smoothScrollToPosition(state.breadcrumbs.size)
                }
            }
            val onClick = View.OnClickListener { v: View ->
                val pos = getChildAdapterPosition(v)
                adapter.currentList.getOrNull(pos)?.uri?.let {
                    browse(it)
                }
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
                this,
                ListingViewAdapter.KeyProvider(adapter),
                ListingViewAdapter.DetailsLookup(this),
                StorageStrategy.createParcelableStorage(Uri::class.java)
            ).withSelectionPredicate(SelectionPredicates.createSelectAnything())
                .withOnItemActivatedListener { item, _ ->
                    item.selectionKey?.let { browse(it) }
                    false // for ripple
                }.build().also {
                    adapter.setSelection(it.selection)
                    SelectionUtils.install(
                        view.findViewById(R.id.browser_top_panel), it, SelectionClient(adapter)
                    )
                }
            model.state.observe(viewLifecycleOwner) { state ->
                val stub = view.findViewById<TextView>(R.id.browser_stub)

                if (isInSearchMode()) {
                    adapter.submitList(state.entries)
                } else {
                    storeCurrentViewPosition()
                    adapter.submitList(state.entries) {
                        stateStorage.currentPath = state.uri
                        restoreCurrentViewPosition()
                    }
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
        model.notification.observe(viewLifecycleOwner) { notification ->
            bind(notification) { intent ->
                runCatching {
                    requireContext().startActivity(intent)
                }.onFailure { showError((it.cause ?: it).message ?: it.javaClass.name) }
            }
        }
    }

    private fun storeCurrentViewPosition() = listingLayoutManager?.findFirstVisibleItemPosition()
        ?.takeIf { it != RecyclerView.NO_POSITION }?.let { pos ->
            stateStorage.currentViewPosition = pos
        }

    private fun restoreCurrentViewPosition() = listingLayoutManager?.scrollToPositionWithOffset(
        stateStorage.currentViewPosition, 0
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
                    model.filter(query)
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
            R.plurals.items, count, count
        )

        override val allItems
            get() = adapter.currentList.map(ListingEntry::uri)

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
                putParcelableArray("uris", convertSelection(selection))
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

    private fun moveUp() =
        search?.takeIf { !it.isIconified }?.let { it.isIconified = true } ?: browseParent()

    private fun browseParent() = model.browseParent()

    private fun isInSearchMode() = true == search?.isFocused

    companion object {
        private const val SEARCH_QUERY_KEY = "search_query"

        private fun convertSelection(selection: Selection<Uri>) =
            selection.iterator().let { iterator ->
                Array<Uri>(selection.size()) {
                    iterator.next()
                }
            }
    }
}
