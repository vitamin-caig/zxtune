package app.zxtune.ui.browser

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Bitmap
import android.net.Uri
import android.util.SparseIntArray
import android.view.Gravity
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.core.util.size
import androidx.core.view.isVisible
import androidx.core.view.updateLayoutParams
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.selection.ItemDetailsLookup
import androidx.recyclerview.selection.ItemDetailsLookup.ItemDetails
import androidx.recyclerview.selection.ItemKeyProvider
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.coverart.BitmapLoader
import app.zxtune.coverart.CoverartProviderClient
import app.zxtune.databinding.BrowserListingEntryBinding
import app.zxtune.ui.utils.await
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.Job
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeoutOrNull
import kotlin.time.Duration.Companion.milliseconds

internal class ListingViewAdapter(ctx: Context) :
    ListAdapter<ListingEntry, ListingViewAdapter.ViewHolder>(DiffCallback()) {
    private val positionsCache = SparseIntArray()
    private lateinit var selection: Selection<Uri>
    private var lastContent: List<ListingEntry> = emptyList()

    @OptIn(ExperimentalCoroutinesApi::class)
    private val dispatcher = Dispatchers.IO.limitedParallelism(6)
    private val icons = BitmapLoader(
        "browser", ctx, maxUsedMemory = 1_048_576 * 5, readDispatcher = dispatcher
    )

    private lateinit var scope: CoroutineScope

    init {
        setHasStableIds(true)
    }

    override fun onAttachedToRecyclerView(rv: RecyclerView) {
        scope = MainScope()
    }

    override fun onDetachedFromRecyclerView(rv: RecyclerView) {
        scope.cancel()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding: BrowserListingEntryBinding = DataBindingUtil.inflate(
            inflater, R.layout.browser_listing_entry, parent, false
        )
        return ViewHolder(binding)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) =
        getItem(position).let { item ->
            holder.bind(item)
        }

    override fun onViewDetachedFromWindow(holder: ViewHolder) {
        holder.unbind()
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
        var pos = positionsCache.size
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

    internal inner class ViewHolder(
        internal val binding: BrowserListingEntryBinding
    ) : RecyclerView.ViewHolder(binding.root) {
        private var loadImageJob: Job? = null

        fun bind(entry: ListingEntry) {
            unbind()
            with(binding) {
                this.entry = entry
                executePendingBindings()
            }
            itemView.isSelected = isSelected(entry.uri)
            when (entry.icon) {
                null, is ListingEntry.DrawableIcon -> setImage(null)
                is ListingEntry.LoadableIcon -> loadImage(entry.icon.uri)
            }
        }

        private fun loadImage(uri: Uri) {
            val imageUri = CoverartProviderClient.getIconUriFor(uri)
            if (!loadCached(imageUri)) {
                loadImageJob = loadRemote(imageUri)
            }
        }

        private fun loadCached(imageUri: Uri) = icons.getCached(imageUri)?.run {
            setImage(bitmap)
        } != null

        private fun loadRemote(imageUri: Uri) = scope.launch {
            val loaded = async(dispatcher) {
                icons.load(imageUri)
            }
            withTimeoutOrNull(15.milliseconds) { loaded.await() }?.let { image ->
                setImage(image.bitmap)
            } ?: run {
                setImage(null)
                loaded.await().bitmap?.let {
                    switchToImage(it)
                }
            }
        }

        fun unbind() {
            loadImageJob?.run {
                cancel()
                loadImageJob = null
            }
        }

        @SuppressLint("RtlHardcoded")
        private fun setImage(image: Bitmap?) = with(binding) {
            browserEntryIcon.alpha = 1f
            image?.let {
                browserEntryMainImage.run {
                    setImageBitmap(it)
                }
            }
            val useImage = image != null
            browserEntryMainImage.isVisible = useImage
            browserEntryMainIcon.isVisible = !useImage
            browserEntryAdditionalIcon.updateLayoutParams<FrameLayout.LayoutParams> {
                gravity = if (useImage) (Gravity.BOTTOM or Gravity.LEFT) else Gravity.CENTER
            }
        }

        private suspend fun switchToImage(image: Bitmap) =
            binding.browserEntryIcon.animate().alpha(0f).await().apply {
                setImage(image)
            }.alpha(1f).await()
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

    private class DiffCallback : DiffUtil.ItemCallback<ListingEntry>() {
        override fun areItemsTheSame(oldItem: ListingEntry, newItem: ListingEntry) =
            oldItem.uri == newItem.uri

        override fun areContentsTheSame(oldItem: ListingEntry, newItem: ListingEntry) =
            oldItem == newItem
    }
}
