package app.zxtune.ui.browser

import android.net.Uri
import android.util.SparseIntArray
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.ViewGroup
import android.widget.Filter
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.selection.ItemDetailsLookup
import androidx.recyclerview.selection.ItemDetailsLookup.ItemDetails
import androidx.recyclerview.selection.ItemKeyProvider
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.databinding.BrowserListingEntryBinding

internal class ListingViewAdapter :
    ListAdapter<ListingEntry, ListingViewAdapter.ViewHolder>(DiffCallback()) {
    private val positionsCache = SparseIntArray()
    private val filter = SearchFilter()
    private lateinit var selection: Selection<Uri>
    private var lastContent: List<ListingEntry> = emptyList()

    init {
        setHasStableIds(true)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding: BrowserListingEntryBinding = DataBindingUtil.inflate(
            inflater,
            R.layout.browser_listing_entry,
            parent, false
        )
        return ViewHolder(binding)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) = getItem(position).let {
        holder.bind(it, isSelected(it.uri))
    }

    fun setSelection(selection: Selection<Uri>) {
        this.selection = selection
    }

    private fun isSelected(uri: Uri) = selection.contains(uri)

    override fun submitList(entries: List<ListingEntry>?, cb: Runnable?) {
        positionsCache.clear()
        lastContent = if (entries !== lastContent) {
            entries ?: emptyList()
        } else {
            ArrayList(entries)
        }
        super.submitList(lastContent, cb)
    }

    fun setFilter(pattern: String) = filter.filter(pattern)

    override fun submitList(entries: List<ListingEntry>?) {
        positionsCache.clear()
        super.submitList(entries)
    }

    override fun getItemId(position: Int) = getItemInternalId(position).toLong()

    fun getItemUri(position: Int) = getItem(position).uri

    private fun getItemInternalId(position: Int) = getItemUri(position).hashCode()

    private fun getItemPosition(uri: Uri): Int {
        val key = uri.hashCode()
        val cached = positionsCache[key, RecyclerView.NO_POSITION]
        if (cached != RecyclerView.NO_POSITION) {
            return cached
        }
        var pos = positionsCache.size()
        val lim = itemCount
        while (pos < lim) {
            val id = getItemInternalId(pos)
            positionsCache.append(id, pos)
            if (key == id) {
                return pos
            }
            ++pos
        }
        return RecyclerView.NO_POSITION
    }

    internal class ViewHolder(internal val binding: BrowserListingEntryBinding) :
        RecyclerView.ViewHolder(binding.root) {
        fun bind(entry: ListingEntry, isSelected: Boolean) {
            binding.entry = entry
            binding.executePendingBindings()
            itemView.isSelected = isSelected
        }
    }

    internal class KeyProvider(private val adapter: ListingViewAdapter) :
        ItemKeyProvider<Uri>(SCOPE_MAPPED) {
        override fun getKey(position: Int) = adapter.getItemUri(position)

        override fun getPosition(key: Uri) = adapter.getItemPosition(key)
    }

    private class HolderItemDetails(private val holder: ViewHolder) : ItemDetails<Uri>() {
        override fun getPosition() = holder.bindingAdapterPosition

        override fun getSelectionKey() = holder.binding.entry?.uri
    }

    internal class DetailsLookup(private val listing: RecyclerView) : ItemDetailsLookup<Uri>() {
        override fun getItemDetails(e: MotionEvent): ItemDetails<Uri>? =
            listing.findChildViewUnder(e.x, e.y)?.let {
                val holder = listing.getChildViewHolder(it) as ViewHolder
                HolderItemDetails(holder)
            }
    }

    private inner class SearchFilter : Filter() {
        override fun performFiltering(constraint: CharSequence) = FilterResults().apply {
            val pattern = constraint.trim()
            values = if (pattern.isEmpty()) {
                lastContent
            } else {
                lastContent.filter {
                    it.title.contains(
                        pattern,
                        ignoreCase = true
                    ) || it.description.contains(pattern, ignoreCase = true)
                }
            }
        }

        @Suppress("UNCHECKED_CAST")
        override fun publishResults(constraint: CharSequence, results: FilterResults) =
            (results.values as? List<ListingEntry>)?.let {
                submitList(it)
            } ?: Unit
    }

    private class DiffCallback : DiffUtil.ItemCallback<ListingEntry>() {
        override fun areItemsTheSame(oldItem: ListingEntry, newItem: ListingEntry) =
            oldItem.uri == newItem.uri

        override fun areContentsTheSame(oldItem: ListingEntry, newItem: ListingEntry) =
            oldItem == newItem
    }
}