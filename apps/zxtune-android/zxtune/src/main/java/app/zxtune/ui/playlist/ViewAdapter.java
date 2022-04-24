package app.zxtune.ui.playlist;

import android.graphics.Rect;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.collection.LongSparseArray;
import androidx.databinding.DataBindingUtil;
import androidx.recyclerview.selection.ItemDetailsLookup;
import androidx.recyclerview.selection.ItemKeyProvider;
import androidx.recyclerview.selection.Selection;
import androidx.recyclerview.widget.DiffUtil;
import androidx.recyclerview.widget.ListAdapter;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import app.zxtune.R;
import app.zxtune.databinding.PlaylistEntryBinding;

class ViewAdapter extends ListAdapter<Entry, ViewAdapter.EntryViewHolder> {

  interface Client {
    boolean onDrag(RecyclerView.ViewHolder holder);
  }

  private final Client client;
  private final LongSparseArray<Integer> positionsCache;
  @Nullable
  private List<Entry> list;
  @Nullable
  private Selection<Long> selection;
  private boolean isPlaying = false;
  @Nullable
  private Long nowPlaying = null;
  @Nullable
  private Integer nowPlayingPos = null;

  ViewAdapter(Client client) {
    super(new DiffUtil.ItemCallback<Entry>() {
      @Override
      public boolean areItemsTheSame(Entry oldItem, Entry newItem) {
        return oldItem.getId() == newItem.getId();
      }

      @Override
      public boolean areContentsTheSame(Entry oldItem, Entry newItem) {
        return TextUtils.equals(oldItem.getTitle(), newItem.getTitle())
            && TextUtils.equals(oldItem.getAuthor(), newItem.getAuthor())
            && oldItem.getDuration().equals(newItem.getDuration());
      }
    });
    this.client = client;
    this.positionsCache = new LongSparseArray<>();
    setHasStableIds(true);
  }

  final void setSelection(@Nullable Selection<Long> selection) {
    this.selection = selection;
  }

  final void setIsPlaying(boolean isPlaying) {
    if (this.isPlaying != isPlaying) {
      updateNowPlaying();
      this.isPlaying = isPlaying;
    }
  }

  final void setNowPlaying(@Nullable Long id) {
    if (nowPlaying != null && !nowPlaying.equals(id)) {
      updateNowPlaying();
    }
    nowPlaying = id;
    nowPlayingPos = getPosition(nowPlaying);
    updateNowPlaying();
  }

  private void updateNowPlaying() {
    if (nowPlayingPos != null) {
      notifyItemChanged(nowPlayingPos);
    }
  }

  @Nullable
  private Integer getPosition(@Nullable Long id) {
    if (id == null) {
      return null;
    }
    final Integer cached = positionsCache.get(id);
    if (cached != null) {
      return cached;
    }
    //TODO: lookup?
    final long numId = id;
    for (int pos = positionsCache.size(), lim = getItemCount(); pos < lim; ++pos) {
      final long itemId = getItem(pos).getId();
      positionsCache.append(itemId, pos);
      if (numId == itemId) {
        return pos;
      }
    }
    return null;
  }

  final void onItemMove(int fromPosition, int toPosition) {
    if (fromPosition == toPosition) {
      return;
    }
    if (fromPosition < toPosition) {
      moveForward(fromPosition, toPosition);
    } else {
      moveBackward(fromPosition, toPosition);
    }
    notifyItemMoved(fromPosition, toPosition);
  }

  private void moveForward(int fromPosition, int toPosition) {
    final Entry moved = list.get(fromPosition);
    for (int pos = fromPosition; pos < toPosition; ++pos) {
      placeItem(pos, list.get(pos + 1));
    }
    placeItem(toPosition, moved);
  }

  private void moveBackward(int fromPosition, int toPosition) {
    final Entry moved = list.get(fromPosition);
    for (int pos = fromPosition; pos > toPosition; --pos) {
      placeItem(pos, list.get(pos - 1));
    }
    placeItem(toPosition, moved);
  }

  private void placeItem(int pos, Entry entry) {
    list.set(pos, entry);
    positionsCache.put(entry.getId(), pos);
  }

  @Override
  public void submitList(@Nullable List<Entry> list) {
    this.list = list;
    this.positionsCache.clear();
    super.submitList(list);
  }

  @Override
  public long getItemId(int position) {
    return getItem(position).getId();
  }

  @Override
  public EntryViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final PlaylistEntryBinding binding = DataBindingUtil.inflate(inflater, R.layout.playlist_entry, parent, false);
    return new EntryViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(final EntryViewHolder holder, int position) {
    final Entry entry = getItem(position);
    holder.bind(entry, isPlaying && isNowPlaying(entry.getId()), isSelected(entry.getId()));
  }

  private boolean isNowPlaying(long id) {
    return nowPlaying != null && nowPlaying == id;
  }

  private boolean isSelected(long id) {
    return selection != null && selection.contains(id);
  }

  @Override
  public void onViewAttachedToWindow(final EntryViewHolder holder) {
    holder.binding.playlistEntryState.setOnTouchListener((v, event) -> {
      if (event.getAction() == MotionEvent.ACTION_DOWN) {
        return client.onDrag(holder);
      }
      return false;
    });
  }

  @Override
  public void onViewDetachedFromWindow(EntryViewHolder holder) {
    holder.binding.playlistEntryState.setOnTouchListener(null);
  }

  static class EntryViewHolder extends RecyclerView.ViewHolder {
    private final PlaylistEntryBinding binding;

    EntryViewHolder(PlaylistEntryBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(Entry entry, boolean isPlaying, boolean isSelected) {
      binding.setEntry(entry);
      binding.setIsPlaying(isPlaying);
      binding.executePendingBindings();
      itemView.setSelected(isSelected);
    }
  }

  static class HolderItemDetails extends ItemDetailsLookup.ItemDetails<Long> {

    private final RecyclerView.ViewHolder holder;

    HolderItemDetails(RecyclerView.ViewHolder holder) {
      this.holder = holder;
    }

    @Override
    public int getPosition() {
      return holder.getAdapterPosition();
    }

    @Nullable
    @Override
    public Long getSelectionKey() {
      final long id = holder.getItemId();
      return id != RecyclerView.NO_ID ? id : null;
    }
  }

  static class KeyProvider extends ItemKeyProvider<Long> {

    private final ViewAdapter adapter;

    KeyProvider(ViewAdapter adapter) {
      super(SCOPE_MAPPED);
      this.adapter = adapter;
    }

    @Nullable
    @Override
    public Long getKey(int position) {
      return adapter.getItem(position).getId();
    }

    @Override
    public int getPosition(Long key) {
      final Integer pos = adapter.getPosition(key);
      return pos != null ? pos : RecyclerView.NO_POSITION;
    }
  }

  static class DetailsLookup extends ItemDetailsLookup<Long> {

    private final RecyclerView listing;

    DetailsLookup(RecyclerView view) {
      this.listing = view;
    }

    @Nullable
    @Override
    public ItemDetails<Long> getItemDetails(MotionEvent e) {
      float x = e.getX();
      float y = e.getY();
      final View item = listing.findChildViewUnder(x, y);
      if (item != null) {
        final EntryViewHolder holder = (EntryViewHolder) listing.getChildViewHolder(item);
        final Rect rect = new Rect();
        holder.binding.playlistEntryState.getHitRect(rect);
        x -= item.getX();
        y -= item.getY();
        // return detail only if not d'n'd event
        return rect.contains((int) x, (int) y)
            ? null
            : new HolderItemDetails(holder);
      }
      return null;
    }
  }
}
