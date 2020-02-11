package app.zxtune.ui.browser;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.databinding.DataBindingUtil;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import app.zxtune.R;
import app.zxtune.databinding.BrowserBreadcrumbBinding;

public class BreadcrumbsViewAdapter extends ListAdapter<BrowserEntrySimple,
                                                           BreadcrumbsViewAdapter.ViewHolder> {

  public BreadcrumbsViewAdapter() {
    super(new DiffUtil.ItemCallback<BrowserEntrySimple>() {
      @Override
      public boolean areItemsTheSame(@NonNull BrowserEntrySimple oldItem, @NonNull BrowserEntrySimple newItem) {
        return oldItem.uri.equals(newItem.uri);
      }

      @Override
      public boolean areContentsTheSame(@NonNull BrowserEntrySimple oldItem, @NonNull BrowserEntrySimple newItem) {
        return oldItem.title.equals(newItem.title)
                   && oldItem.icon == newItem.icon;
      }
    });
    setHasStableIds(true);
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final BrowserBreadcrumbBinding binding = DataBindingUtil.inflate(inflater,
        R.layout.browser_breadcrumb,
        parent, false);
    return new ViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
    final BrowserEntrySimple entry = getItem(position);
    holder.bind(entry);
  }

  @Override
  public long getItemId(int position) {
    final BrowserEntrySimple entry = getItem(position);
    return entry != null ? entry.uri.hashCode() : RecyclerView.NO_ID;
  }

  static class ViewHolder extends RecyclerView.ViewHolder {

    private final BrowserBreadcrumbBinding binding;

    ViewHolder(BrowserBreadcrumbBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(@NonNull BrowserEntrySimple entry) {
      binding.setEntry(entry);
      binding.executePendingBindings();
    }
  }
}
