package app.zxtune.ui.playlist

import android.annotation.SuppressLint
import android.graphics.Rect
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.ViewGroup
import androidx.collection.LongSparseArray
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.selection.ItemDetailsLookup
import androidx.recyclerview.selection.ItemDetailsLookup.ItemDetails
import androidx.recyclerview.selection.ItemKeyProvider
import androidx.recyclerview.selection.Selection
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.databinding.PlaylistEntryBinding

internal class ViewAdapter(private val client: Client) :
    ListAdapter<Entry, ViewAdapter.EntryViewHolder>(DiffCallback()) {
    internal fun interface Client {
        fun move(id: Long, delta: Int)
    }

    init {
        setHasStableIds(true)
    }

    private val positionsCache: LongSparseArray<Int> = LongSparseArray()
    private lateinit var mutableList: MutableList<Entry>
    private lateinit var selection: Selection<Long>
    private lateinit var touchHelper: CustomTouchHelper
    private var isPlaying = false
    private var nowPlaying: Long? = null
    private var nowPlayingPos: Int? = null

    fun setSelection(selection: Selection<Long>) {
        this.selection = selection
    }

    fun setIsPlaying(isPlaying: Boolean) {
        if (this.isPlaying != isPlaying) {
            updateNowPlaying()
            this.isPlaying = isPlaying
        }
    }

    fun setNowPlaying(id: Long?) {
        if (nowPlaying != null && nowPlaying != id) {
            updateNowPlaying()
        }
        nowPlaying = id
        nowPlayingPos = id?.let { getPosition(it) }
        updateNowPlaying()
    }

    private fun updateNowPlaying() = nowPlayingPos?.let {
        notifyItemChanged(it)
    } ?: Unit

    private fun getPosition(id: Long): Int? {
        positionsCache[id]?.let {
            return it
        }
        //TODO: lookup?
        for (pos in positionsCache.size() until itemCount) {
            val itemId = getItem(pos).id
            positionsCache.append(itemId, pos)
            if (id == itemId) {
                return pos
            }
        }
        return null
    }

    fun onItemMove(fromPosition: Int, toPosition: Int) {
        if (fromPosition == toPosition) {
            return
        }
        if (maxOf(fromPosition, toPosition) >= mutableList.size) {
            return
        }
        val placeItem = { pos: Int, entry: Entry ->
            mutableList[pos] = entry
            positionsCache.put(entry.id, pos)
        }
        val moved = mutableList[fromPosition]
        if (fromPosition < toPosition) {
            for (pos in fromPosition until toPosition) {
                placeItem(pos, mutableList[pos + 1])
            }
        } else {
            for (pos in fromPosition downTo toPosition + 1) {
                placeItem(pos, mutableList[pos - 1])
            }
        }
        placeItem(toPosition, moved)
        notifyItemMoved(fromPosition, toPosition)
    }

    override fun submitList(list: List<Entry>?, callback: Runnable?) {
        if (touchHelper.isDragging) {
            callback?.run()
        } else {
            positionsCache.clear()
            mutableList = (list as? MutableList<Entry>) ?: mutableListOf()
            super.submitList(list, callback)
        }
    }

    override fun getItemId(position: Int) = getItem(position).id

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): EntryViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding: PlaylistEntryBinding =
            DataBindingUtil.inflate(inflater, R.layout.playlist_entry, parent, false)
        return EntryViewHolder(binding)
    }

    override fun onBindViewHolder(holder: EntryViewHolder, position: Int) {
        getItem(position).let { entry ->
            holder.bind(entry, isPlaying && isNowPlaying(entry.id), isSelected(entry.id))
        }
    }

    private fun isNowPlaying(id: Long) = id == nowPlaying

    private fun isSelected(id: Long) = selection.contains(id)

    private fun hasSelection() = !selection.isEmpty

    @SuppressLint("ClickableViewAccessibility")
    override fun onViewAttachedToWindow(holder: EntryViewHolder) {
        holder.binding.playlistEntryState.setOnTouchListener { _, event ->
            if (event.action == MotionEvent.ACTION_DOWN && !hasSelection()) {
                touchHelper.startDrag(holder)
                true
            } else {
                false
            }
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onViewDetachedFromWindow(holder: EntryViewHolder) =
        holder.binding.playlistEntryState.setOnTouchListener(null)

    override fun onAttachedToRecyclerView(recyclerView: RecyclerView) {
        touchHelper = CustomTouchHelper(TouchHelperCallback()).apply {
            attachToRecyclerView(recyclerView)
        }
    }

    private class CustomTouchHelper(private val callback: TouchHelperCallback) :
        ItemTouchHelper(callback) {

        val isDragging
            get() = callback.isDragging
    }

    private inner class TouchHelperCallback :
        ItemTouchHelper.SimpleCallback(ItemTouchHelper.UP or ItemTouchHelper.DOWN, 0) {

        private var draggedItem: Long? = null
        private var dragDelta = 0

        val isDragging
            get() = draggedItem != null

        override fun isLongPressDragEnabled() = false

        override fun isItemViewSwipeEnabled() = false

        override fun onSelectedChanged(
            viewHolder: RecyclerView.ViewHolder?,
            actionState: Int
        ) {
            if (actionState != ItemTouchHelper.ACTION_STATE_IDLE) {
                viewHolder?.apply {
                    itemView.isActivated = true
                }
            }
            super.onSelectedChanged(viewHolder, actionState)
        }

        override fun clearView(view: RecyclerView, viewHolder: RecyclerView.ViewHolder) {
            super.clearView(view, viewHolder)
            viewHolder.itemView.isActivated = false
            draggedItem.takeIf { dragDelta != 0 }?.let {
                client.move(it, dragDelta)
            }
            draggedItem = null
            dragDelta = 0
        }

        override fun onMove(
            recyclerView: RecyclerView,
            source: RecyclerView.ViewHolder,
            target: RecyclerView.ViewHolder
        ): Boolean {
            val srcPos = source.bindingAdapterPosition
            val tgtPos = target.bindingAdapterPosition
            if (draggedItem == null) {
                draggedItem = source.itemId
            }
            dragDelta += tgtPos - srcPos
            onItemMove(srcPos, tgtPos)
            return true
        }

        override fun onSwiped(viewHolder: RecyclerView.ViewHolder, direction: Int) = Unit
    }

    internal class EntryViewHolder(val binding: PlaylistEntryBinding) :
        RecyclerView.ViewHolder(binding.root) {
        fun bind(entry: Entry, isPlaying: Boolean, isSelected: Boolean) {
            binding.entry = entry
            binding.isPlaying = isPlaying
            binding.executePendingBindings()
            itemView.isSelected = isSelected
        }
    }

    internal class HolderItemDetails(private val holder: RecyclerView.ViewHolder) :
        ItemDetails<Long>() {
        override fun getPosition() = holder.bindingAdapterPosition

        override fun getSelectionKey() = holder.itemId
    }

    internal class KeyProvider(private val adapter: ViewAdapter) :
        ItemKeyProvider<Long>(SCOPE_MAPPED) {
        override fun getKey(position: Int) = adapter.getItem(position).id

        override fun getPosition(key: Long) = adapter.getPosition(key) ?: RecyclerView.NO_POSITION
    }

    internal class DetailsLookup(private val listing: RecyclerView, private val adapter : ViewAdapter) : ItemDetailsLookup<Long>
        () {
        override fun getItemDetails(e: MotionEvent): ItemDetails<Long>? {
            if (adapter.touchHelper.isDragging) {
                return null
            }
            var x = e.x
            var y = e.y
            val item = listing.findChildViewUnder(x, y)
            if (item != null) {
                val holder = listing.getChildViewHolder(item) as EntryViewHolder
                val rect = Rect()
                holder.binding.playlistEntryState.getHitRect(rect)
                x -= item.x
                y -= item.y
                // return detail only if not d'n'd event
                return if (rect.contains(x.toInt(), y.toInt())) null else HolderItemDetails(holder)
            }
            return null
        }
    }

    private class DiffCallback : DiffUtil.ItemCallback<Entry>() {
        override fun areItemsTheSame(oldItem: Entry, newItem: Entry) = oldItem.id == newItem.id

        override fun areContentsTheSame(oldItem: Entry, newItem: Entry) = oldItem == newItem
    }
}