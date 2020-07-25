package app.zxtune.ui.browser;

import android.view.LayoutInflater;
import android.view.ViewGroup;

import androidx.core.util.ObjectsCompat;
import androidx.databinding.DataBindingUtil;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import app.zxtune.R;
import app.zxtune.databinding.BrowserBreadcrumbsEntryBinding;

class BreadcrumbsViewAdapter extends ListAdapter<BreadcrumbsEntry,
    BreadcrumbsViewAdapter.ViewHolder> {

  BreadcrumbsViewAdapter() {
    super(new DiffUtil.ItemCallback<BreadcrumbsEntry>() {
      @Override
      public boolean areItemsTheSame(BreadcrumbsEntry oldItem, BreadcrumbsEntry newItem) {
        return oldItem.uri.equals(newItem.uri);
      }

      @Override
      public boolean areContentsTheSame(BreadcrumbsEntry oldItem, BreadcrumbsEntry newItem) {
        return oldItem.title.equals(newItem.title)
            && ObjectsCompat.equals(oldItem.icon, newItem.icon);
      }
    });
    setHasStableIds(true);
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final BrowserBreadcrumbsEntryBinding binding = DataBindingUtil.inflate(inflater,
        R.layout.browser_breadcrumbs_entry,
        parent, false);
    return new ViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position) {
    final BreadcrumbsEntry entry = getItem(position);
    holder.bind(entry);
  }

  @Override
  public long getItemId(int position) {
    final BreadcrumbsEntry entry = getItem(position);
    return entry != null ? entry.uri.hashCode() : RecyclerView.NO_ID;
  }

  static class ViewHolder extends RecyclerView.ViewHolder {

    private final BrowserBreadcrumbsEntryBinding binding;

    ViewHolder(BrowserBreadcrumbsEntryBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(BreadcrumbsEntry entry) {
      binding.setEntry(entry);
      binding.executePendingBindings();
    }
  }
}
