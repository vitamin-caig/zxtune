package app.zxtune.ui.browser;

import android.net.Uri;
import android.util.SparseIntArray;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
import app.zxtune.databinding.BrowserEntryBinding;

public class ListingViewAdapter extends ListAdapter<BrowserEntry,
                                                       ListingViewAdapter.ViewHolder> {

  private final SparseIntArray positionsCache;
  private Selection<Uri> selection;
  private List<BrowserEntry> lastContent;

  public ListingViewAdapter() {
    super(new DiffUtil.ItemCallback<BrowserEntry>() {
      @Override
      public boolean areItemsTheSame(@NonNull BrowserEntry oldItem, @NonNull BrowserEntry newItem) {
        return oldItem.uri.equals(newItem.uri);
      }

      @Override
      public boolean areContentsTheSame(@NonNull BrowserEntry oldItem, @NonNull BrowserEntry newItem) {
        return oldItem.type == newItem.type
                   && oldItem.icon == newItem.icon
                   && oldItem.title.equals(newItem.title)
                   && oldItem.description.equals(newItem.description)
                   && oldItem.details.equals(newItem.details);
      }
    });
    positionsCache = new SparseIntArray();
    setHasStableIds(true);
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final BrowserEntryBinding binding = DataBindingUtil.inflate(inflater,
        R.layout.browser_entry,
        parent, false);
    return new ViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
    final BrowserEntry entry = getItem(position);
    holder.bind(entry, isSelected(entry.uri));
  }

  public final void setSelection(Selection<Uri> selection) {
    this.selection = selection;
  }

  private boolean isSelected(Uri uri) {
    return selection != null && selection.contains(uri);
  }

  @Override
  public void submitList(List<BrowserEntry> entries, Runnable cb) {
    positionsCache.clear();
    lastContent = entries == lastContent ? new ArrayList<>(entries) : entries;
    super.submitList(lastContent, cb);
  }

  @Override
  public long getItemId(int position) {
    return getItemInternalId(position);
  }

  public final Uri getItemUri(int position) {
    final BrowserEntry entry = getItem(position);
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

    private final BrowserEntryBinding binding;

    ViewHolder(BrowserEntryBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(@NonNull BrowserEntry entry, boolean isSelected) {
      binding.setEntry(entry);
      binding.executePendingBindings();
      itemView.setSelected(isSelected);
    }
  }

  public static class KeyProvider extends ItemKeyProvider<Uri> {

    private final ListingViewAdapter adapter;

    public KeyProvider(ListingViewAdapter adapter) {
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

  public static class DetailsLookup extends ItemDetailsLookup<Uri> {

    private final RecyclerView listing;

    public DetailsLookup(RecyclerView view) {
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
}
