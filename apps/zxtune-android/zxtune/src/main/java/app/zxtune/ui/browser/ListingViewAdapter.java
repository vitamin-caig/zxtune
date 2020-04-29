package app.zxtune.ui.browser;

import android.net.Uri;
import android.text.TextUtils;
import android.util.SparseIntArray;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Filter;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.util.ObjectsCompat;
import androidx.databinding.DataBindingUtil;
import androidx.recyclerview.selection.ItemDetailsLookup;
import androidx.recyclerview.selection.ItemKeyProvider;
import androidx.recyclerview.selection.Selection;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

import app.zxtune.R;
import app.zxtune.databinding.BrowserListingEntryBinding;

class ListingViewAdapter extends ListAdapter<ListingEntry,
    ListingViewAdapter.ViewHolder> {

  private final SparseIntArray positionsCache;
  private final SearchFilter filter;
  private Selection<Uri> selection;
  private List<ListingEntry> lastContent;

  ListingViewAdapter() {
    super(new DiffUtil.ItemCallback<ListingEntry>() {
      @Override
      public boolean areItemsTheSame(@NonNull ListingEntry oldItem, @NonNull ListingEntry newItem) {
        return oldItem.uri.equals(newItem.uri);
      }

      @Override
      public boolean areContentsTheSame(@NonNull ListingEntry oldItem, @NonNull ListingEntry newItem) {
        return oldItem.type == newItem.type
            && oldItem.icon == newItem.icon
            && TextUtils.equals(oldItem.title, newItem.title)
            && TextUtils.equals(oldItem.description, newItem.description)
            && TextUtils.equals(oldItem.details, newItem.details)
            && ObjectsCompat.equals(oldItem.tracks, newItem.tracks)
            && ObjectsCompat.equals(oldItem.cached, newItem.cached);
      }
    });
    positionsCache = new SparseIntArray();
    filter = new SearchFilter();
    setHasStableIds(true);
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final BrowserListingEntryBinding binding = DataBindingUtil.inflate(inflater,
        R.layout.browser_listing_entry,
        parent, false);
    return new ViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
    final ListingEntry entry = getItem(position);
    holder.bind(entry, isSelected(entry.uri));
  }

  final void setSelection(Selection<Uri> selection) {
    this.selection = selection;
  }

  private boolean isSelected(Uri uri) {
    return selection != null && selection.contains(uri);
  }

  @Override
  public void submitList(List<ListingEntry> entries, Runnable cb) {
    positionsCache.clear();
    lastContent = entries == lastContent ? new ArrayList<>(entries) : entries;
    super.submitList(lastContent, cb);
  }

  final void setFilter(String pattern) {
    filter.filter(pattern);
  }

  @Override
  public void submitList(List<ListingEntry> entries) {
    positionsCache.clear();
    super.submitList(entries);
  }

  @Override
  public long getItemId(int position) {
    return getItemInternalId(position);
  }

  final Uri getItemUri(int position) {
    final ListingEntry entry = getItem(position);
    return entry.uri;
  }

  private int getItemInternalId(int position) {
    return getItemUri(position).hashCode();
  }

  private int getItemPosition(@Nullable Uri uri) {
    if (uri == null) {
      return RecyclerView.NO_POSITION;
    }
    final int key = uri.hashCode();
    final int cached = positionsCache.get(key, RecyclerView.NO_POSITION);
    if (cached != RecyclerView.NO_POSITION) {
      return cached;
    }
    for (int pos = positionsCache.size(), lim = getItemCount(); pos < lim; ++pos) {
      final int id = getItemInternalId(pos);
      positionsCache.append(id, pos);
      if (key == id) {
        return pos;
      }
    }
    return RecyclerView.NO_POSITION;
  }

  static class ViewHolder extends RecyclerView.ViewHolder {

    private final BrowserListingEntryBinding binding;

    ViewHolder(BrowserListingEntryBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(@NonNull ListingEntry entry, boolean isSelected) {
      binding.setEntry(entry);
      binding.executePendingBindings();
      itemView.setSelected(isSelected);
    }
  }

  static class KeyProvider extends ItemKeyProvider<Uri> {

    private final ListingViewAdapter adapter;

    KeyProvider(ListingViewAdapter adapter) {
      super(SCOPE_MAPPED);
      this.adapter = adapter;
    }

    @Nullable
    @Override
    public Uri getKey(int position) {
      return adapter.getItemUri(position);
    }

    @Override
    public int getPosition(@NonNull Uri key) {
      return adapter.getItemPosition(key);
    }
  }

  static class HolderItemDetails extends ItemDetailsLookup.ItemDetails<Uri> {

    private final ViewHolder holder;

    HolderItemDetails(ViewHolder holder) {
      this.holder = holder;
    }


    @Override
    public int getPosition() {
      return holder.getAdapterPosition();
    }

    @Nullable
    @Override
    public Uri getSelectionKey() {
      return holder.binding.getEntry().uri;
    }
  }

  static class DetailsLookup extends ItemDetailsLookup<Uri> {

    private final RecyclerView listing;

    DetailsLookup(RecyclerView view) {
      this.listing = view;
    }

    @Nullable
    @Override
    public ItemDetails<Uri> getItemDetails(@NonNull MotionEvent e) {
      final View item = listing.findChildViewUnder(e.getX(), e.getY());
      if (item != null) {
        final ViewHolder holder = (ViewHolder) listing.getChildViewHolder(item);
        return new HolderItemDetails(holder);
      }
      return null;
    }
  }

  private class SearchFilter extends Filter {

    @Override
    protected FilterResults performFiltering(CharSequence constraint) {
      final FilterResults result = new FilterResults();
      if (TextUtils.isEmpty(constraint)) {
        result.count = lastContent.size();
        result.values = lastContent;
      } else {
        final ArrayList<ListingEntry> filtered = new ArrayList<>();
        final String pattern = constraint.toString().toLowerCase().trim();
        for (ListingEntry entry : lastContent) {
          if (entry.title.toLowerCase().contains(pattern) || entry.description.toLowerCase().contains(pattern)) {
            filtered.add(entry);
          }
        }
        result.count = filtered.size();
        result.values = filtered;
      }
      return result;
    }

    @Override
    protected void publishResults(CharSequence constraint, FilterResults results) {
      submitList((List<ListingEntry>) results.values);
    }
  }
}
