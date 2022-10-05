/**
 * @file
 * @brief Playlist fragment component
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.playlist

import android.net.Uri
import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.*
import android.widget.SearchView
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.annotation.VisibleForTesting
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.selection.SelectionPredicates
import androidx.recyclerview.selection.SelectionTracker
import androidx.recyclerview.selection.StorageStrategy
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.device.PersistentStorage
import app.zxtune.device.media.MediaSessionModel
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.utils.SelectionUtils

class PlaylistFragment : Fragment() {
    private lateinit var listing: RecyclerView
    private lateinit var search: SearchView
    private lateinit var selectionTracker: SelectionTracker<Long>

    private val model by lazy {
        Model.of(this)
    }
    private val mediaSessionModel
        get() = MediaSessionModel.of(requireActivity())
    private val mediaController
        get() = MediaControllerCompat.getMediaController(requireActivity())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setHasOptionsMenu(true)
    }

    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        super.onCreateOptionsMenu(menu, inflater)
        inflater.inflate(R.menu.playlist, menu)
        val sortMenuRoot = requireNotNull(menu.findItem(R.id.action_sort)).subMenu
        for (sortBy in ProviderClient.SortBy.values()) {
            for (sortOrder in ProviderClient.SortOrder.values()) {
                sortMenuRoot.add(getMenuTitle(sortBy)).run {
                    setOnMenuItemClickListener {
                        model.sort(sortBy, sortOrder)
                        true
                    }
                    setIcon(getMenuIcon(sortOrder))
                }
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem) = processMenuItem(
        item.itemId,
        selectionTracker.selection
    ) || super.onOptionsItemSelected(item)

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ) = container?.let { inflater.inflate(R.layout.playlist, it, false) }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        listing = setupListing(view)
        search = setupSearchView(view)
    }

    private fun setupListing(view: View) =
        view.findViewById<RecyclerView>(R.id.playlist_content).apply {
            setHasFixedSize(true)
            val adapter = ViewAdapter(model::move).apply {
                adapter = this
            }
            selectionTracker = SelectionTracker.Builder(
                "playlist_selection",
                this,
                ViewAdapter.KeyProvider(adapter),
                ViewAdapter.DetailsLookup(this, adapter),
                StorageStrategy.createLongStorage()
            )
                .withSelectionPredicate(SelectionPredicates.createSelectAnything())
                .withOnItemActivatedListener { item, _ ->
                    item.selectionKey?.let { onItemClick(it) }
                    true
                }.build().also {
                    adapter.setSelection(it.selection)
                    // another class for test
                    (activity as? AppCompatActivity)?.let { activity ->
                        SelectionUtils.install(activity, it, SelectionClient(adapter))
                    }
                }
            model.state.observe(viewLifecycleOwner) { state ->
                adapter.submitList(state.entries) {
                    val stub = view.findViewById<View>(R.id.playlist_stub)
                    if (0 == adapter.itemCount) {
                        visibility = View.GONE
                        stub.visibility = View.VISIBLE
                    } else {
                        visibility = View.VISIBLE
                        stub.visibility = View.GONE
                    }
                }
            }
            mediaSessionModel.run {
                state.observe(viewLifecycleOwner) { state: PlaybackStateCompat? ->
                    adapter.setIsPlaying(PlaybackStateCompat.STATE_PLAYING == state?.state)
                }
                metadata.observe(viewLifecycleOwner) { metadata: MediaMetadataCompat? ->
                    metadata?.let {
                        val uri = Uri.parse(it.description.mediaId)
                        adapter.setNowPlaying(ProviderClient.findId(uri))
                    }
                }
            }
        }

    private fun setupSearchView(view: View) = view.findViewById<SearchView>(R.id.playlist_search)
        .apply {
            isSubmitButtonEnabled = false
            setOnSearchClickListener {
                layoutParams = layoutParams.apply {
                    width = ViewGroup.LayoutParams.MATCH_PARENT
                }
            }
            setOnCloseListener {
                layoutParams = layoutParams.apply {
                    width = ViewGroup.LayoutParams.WRAP_CONTENT
                }
                post {
                    clearFocus()
                }
                false
            }
            onFocusChangeListener = View.OnFocusChangeListener { _, _ ->
                if (!isIconified && query.isEmpty()) {
                    isIconified = true
                }
            }
            setOnQueryTextListener(object : SearchView.OnQueryTextListener {
                override fun onQueryTextSubmit(query: String) = true
                override fun onQueryTextChange(newText: String): Boolean {
                    model.filter(newText)
                    return true
                }
            })
        }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        selectionTracker.onSaveInstanceState(outState)
    }

    override fun onViewStateRestored(savedInstanceState: Bundle?) {
        super.onViewStateRestored(savedInstanceState)
        savedInstanceState?.let { state ->
            selectionTracker.onRestoreInstanceState(state)
        }
        model.state.value?.filter?.takeIf { it.isNotEmpty() }.let { query ->
            search.post {
                search.setQuery(query, false)
            }
        }
    }

    private fun onItemClick(id: Long) =
        mediaController?.transportControls?.playFromUri(ProviderClient.createUri(id), null) ?: Unit

    // ArchivesService for selection
    private inner class SelectionClient(private val adapter: ViewAdapter) :
        SelectionUtils.Client<Long> {
        override fun getTitle(count: Int) =
            resources.getQuantityString(R.plurals.tracks, count, count)

        override fun getAllItems() = adapter.currentList.map(Entry::id)

        override fun fillMenu(inflater: MenuInflater, menu: Menu) =
            inflater.inflate(R.menu.playlist_items, menu)

        override fun processMenu(itemId: Int, selection: Selection<Long>) =
            processMenuItem(itemId, selection)
    }

    private fun processMenuItem(itemId: Int, selection: Selection<Long>): Boolean {
        when (itemId) {
            R.id.action_clear -> deletionAlert(R.string.delete_all_items_query) {
                model.deleteAll()
            }
            R.id.action_delete -> convertSelection(selection)?.let {
                deletionAlert(R.string.delete_selected_items_query) {
                    model.delete(it)
                }
            }
            R.id.action_save -> savePlaylist(convertSelection(selection))
            R.id.action_statistics -> showStatistics(convertSelection(selection))
            else -> return false
        }
        return true
    }

    // TODO: think about using DialogFragment - complicated lambda passing
    private fun deletionAlert(@StringRes message: Int, action: () -> Unit) =
        AlertDialog.Builder(requireContext())
            .setTitle(message)
            .setPositiveButton(R.string.delete) { _, _ ->
                action()
            }
            .show()

    private fun savePlaylist(ids: LongArray?) =
        PlaylistSaveFragment.show(requireActivity(), PersistentStorage.instance, ids)

    private fun showStatistics(ids: LongArray?) =
        PlaylistStatisticsFragment.createInstance(ids).show(parentFragmentManager, "statistics")

    companion object {
        @JvmStatic
        fun createInstance(): Fragment = PlaylistFragment()

        @StringRes
        private fun getMenuTitle(by: ProviderClient.SortBy) = when (by) {
            ProviderClient.SortBy.title -> R.string.information_title
            ProviderClient.SortBy.author -> R.string.information_author
            ProviderClient.SortBy.duration -> R.string.statistics_duration //TODO: extract
        }

        @DrawableRes
        private fun getMenuIcon(order: ProviderClient.SortOrder) = when (order) {
            ProviderClient.SortOrder.asc -> android.R.drawable.arrow_up_float
            ProviderClient.SortOrder.desc -> android.R.drawable.arrow_down_float
        }

        @VisibleForTesting
        fun convertSelection(selection: Selection<Long>) =
            selection.takeUnless { it.isEmpty }?.iterator()?.let { iterator ->
                LongArray(selection.size()) {
                    iterator.next()
                }
            }
    }
}
