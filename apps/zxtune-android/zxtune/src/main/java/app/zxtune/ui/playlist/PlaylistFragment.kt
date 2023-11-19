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
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.annotation.DrawableRes
import androidx.annotation.StringRes
import androidx.annotation.VisibleForTesting
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.widget.SearchView
import androidx.appcompat.widget.Toolbar
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.selection.SelectionPredicates
import androidx.recyclerview.selection.SelectionTracker
import androidx.recyclerview.selection.StorageStrategy
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.device.media.MediaModel
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.PersistentStorageSetupFragment
import app.zxtune.ui.utils.SelectionUtils
import app.zxtune.ui.utils.item
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.launch

class PlaylistFragment : Fragment() {
    private lateinit var listing: RecyclerView
    private lateinit var search: SearchView
    private lateinit var selectionTracker: SelectionTracker<Long>

    private val model by activityViewModels<Model>()
    private val mediaModel
        get() = MediaModel.of(requireActivity())
    private val mediaController
        get() = MediaControllerCompat.getMediaController(requireActivity())

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let { inflater.inflate(R.layout.playlist, it, false) }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        val panel = view.findViewById<FrameLayout>(R.id.playlist_top_panel)
        setupToolbarMenu(panel)
        listing = setupListing(
            panel, view.findViewById(R.id.playlist_content), view.findViewById(R.id.playlist_stub)
        )
        search = setupSearchView(view.findViewById(R.id.playlist_search))
    }

    private fun setupToolbarMenu(panel: FrameLayout) {
        require(panel.childCount == 1)
        val toolbar = panel.getChildAt(0) as Toolbar
        toolbar.addMenuProvider(object : MenuProvider {
            override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {
                menuInflater.inflate(R.menu.playlist, menu)
                // for some reason, onPrepareMenu is not called anymore if menu is shown via
                // showAsAction for some items
                requireNotNull(menu.item(R.id.action_sort).subMenu).run {
                    for (sortBy in ProviderClient.SortBy.values()) {
                        for (sortOrder in ProviderClient.SortOrder.values()) {
                            add(getMenuTitle(sortBy)).run {
                                setOnMenuItemClickListener {
                                    model.sort(sortBy, sortOrder)
                                    true
                                }
                                setIcon(getMenuIcon(sortOrder))
                            }
                        }
                    }
                }
            }

            override fun onMenuItemSelected(menuItem: MenuItem) =
                processMenuItem(menuItem.itemId, selectionTracker.selection)
        })
    }

    private fun setupListing(panel: FrameLayout, listing: RecyclerView, stub: View) =
        listing.apply {
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
            ).withSelectionPredicate(SelectionPredicates.createSelectAnything())
                .withOnItemActivatedListener { item, _ ->
                    item.selectionKey?.let { onItemClick(it) }
                    false // for ripple
                }.build().also {
                    adapter.setSelection(it.selection)
                    SelectionUtils.install(panel, it, SelectionClient(adapter))
                }
            viewLifecycleOwner.whenLifecycleStarted {
                model.state.collect { state ->
                    adapter.submitList(state.entries) {
                        if (0 == adapter.itemCount) {
                            visibility = View.GONE
                            stub.visibility = View.VISIBLE
                        } else {
                            visibility = View.VISIBLE
                            stub.visibility = View.GONE
                        }
                    }
                }
            }
            mediaModel.run {
                playbackState.observe(viewLifecycleOwner) { state: PlaybackStateCompat? ->
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

    private fun setupSearchView(view: SearchView) = view.apply {
        isSubmitButtonEnabled = false
        setOnCloseListener {
            post {
                clearFocus()
            }
            false
        }
        setOnQueryTextFocusChangeListener { _, hasFocus ->
            if (!hasFocus && query.isEmpty()) {
                isIconified = true
            }
        }
        setOnQueryTextListener(object : SearchView.OnQueryTextListener {
            override fun onQueryTextSubmit(query: String) = true
            override fun onQueryTextChange(newText: String): Boolean {
                model.filter = newText
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
        model.filter.takeIf { it.isNotEmpty() }?.let { query ->
            search.post {
                search.setQuery(query, false)
            }
        }
    }

    private fun onItemClick(id: Long) = mediaController?.transportControls?.playFromUri(
        ProviderClient.createUri(id), null
    ) ?: Unit

    // ArchivesService for selection
    private inner class SelectionClient(private val adapter: ViewAdapter) :
        SelectionUtils.Client<Long> {
        override fun getTitle(count: Int) =
            resources.getQuantityString(R.plurals.tracks, count, count)

        override val allItems
            get() = adapter.currentList.map(Entry::id)

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
        AlertDialog.Builder(requireContext()).setTitle(message)
            .setPositiveButton(R.string.delete) { _, _ ->
                action()
            }.show()

    private fun savePlaylist(ids: LongArray?) = lifecycleScope.launch {
        val persistentStorageSetupAction =
            VfsProviderClient(requireContext()).getNotification(Uri.parse("playlists:/"))?.action
        val fragment = persistentStorageSetupAction?.let {
            PersistentStorageSetupFragment.createInstance(it)
        } ?: PlaylistSaveFragment.createInstance(ids)
        fragment.show(parentFragmentManager, "save")
    }

    private fun showStatistics(ids: LongArray?) =
        PlaylistStatisticsFragment.createInstance(ids).show(parentFragmentManager, "statistics")

    companion object {
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
