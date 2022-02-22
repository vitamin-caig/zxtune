package app.zxtune.ui.browser

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.databinding.BrowserBreadcrumbsEntryBinding

internal class BreadcrumbsViewAdapter :
    ListAdapter<BreadcrumbsEntry, BreadcrumbsViewAdapter.ViewHolder>(DiffCallback()) {

    init {
        setHasStableIds(true)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding: BrowserBreadcrumbsEntryBinding = DataBindingUtil.inflate(
            inflater,
            R.layout.browser_breadcrumbs_entry,
            parent, false
        )
        return ViewHolder(binding)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) = getItem(position).let {
        holder.bind(it)
    }

    override fun getItemId(position: Int) = getItem(position).uri.hashCode().toLong()

    internal class ViewHolder(private val binding: BrowserBreadcrumbsEntryBinding) :
        RecyclerView.ViewHolder(binding.root) {
        fun bind(entry: BreadcrumbsEntry) {
            binding.entry = entry
            binding.executePendingBindings()
        }
    }

    private class DiffCallback : DiffUtil.ItemCallback<BreadcrumbsEntry>() {
        override fun areItemsTheSame(oldItem: BreadcrumbsEntry, newItem: BreadcrumbsEntry) =
            oldItem.uri == newItem.uri

        override fun areContentsTheSame(oldItem: BreadcrumbsEntry, newItem: BreadcrumbsEntry) =
            oldItem == newItem
    }
}
