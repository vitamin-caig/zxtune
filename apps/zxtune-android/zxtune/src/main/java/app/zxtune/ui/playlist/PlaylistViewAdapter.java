package app.zxtune.ui.playlist;

import android.graphics.Rect;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
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

public class PlaylistViewAdapter extends ListAdapter<PlaylistEntry, PlaylistViewAdapter.EntryViewHolder> {

  public interface Client {
    boolean onDrag(@NonNull RecyclerView.ViewHolder holder);
  }

  private final Client client;
  private final LongSparseArray<Integer> positionsCache;
  private List<PlaylistEntry> list;
  private Selection<Long> selection;
  private boolean isPlaying = false;
  private Long nowPlaying = null;
  private Integer nowPlayingPos = null;

  public PlaylistViewAdapter(Client client) {
    super(new DiffUtil.ItemCallback<PlaylistEntry>() {
      @Override
      public boolean areItemsTheSame(@NonNull PlaylistEntry oldItem, @NonNull PlaylistEntry newItem) {
        return oldItem.id == newItem.id;
      }

      @Override
      public boolean areContentsTheSame(@NonNull PlaylistEntry oldItem, @NonNull PlaylistEntry newItem) {
        return oldItem.title.equals(newItem.title)
                   && oldItem.author.equals(newItem.author)
                   && 0 == oldItem.duration.compareTo(newItem.duration);
      }
    });
    this.client = client;
    this.positionsCache = new LongSparseArray<>();
    setHasStableIds(true);
  }

  public final void setSelection(@Nullable Selection<Long> selection) {
    this.selection = selection;
  }

  public final void setIsPlaying(boolean isPlaying) {
    if (this.isPlaying != isPlaying) {
      updateNowPlaying();
      this.isPlaying = isPlaying;
    }
  }

  public final void setNowPlaying(@Nullable Long id) {
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
      final long itemId = getItem(pos).id;
      positionsCache.append(itemId, pos);
      if (numId == itemId) {
        return pos;
      }
    }
    return null;
  }

  public final void onItemMove(int fromPosition, int toPosition) {
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
    final PlaylistEntry moved = list.get(fromPosition);
    for (int pos = fromPosition; pos < toPosition; ++pos) {
      placeItem(pos, list.get(pos + 1));
    }
    placeItem(toPosition, moved);
  }

  private void moveBackward(int fromPosition, int toPosition) {
    final PlaylistEntry moved = list.get(fromPosition);
    for (int pos = fromPosition; pos > toPosition; --pos) {
      placeItem(pos, list.get(pos - 1));
    }
    placeItem(toPosition, moved);
  }

  private void placeItem(int pos, PlaylistEntry entry) {
    list.set(pos, entry);
    positionsCache.put(entry.id, pos);
  }

  @Override
  public void submitList(List<PlaylistEntry> list) {
    this.list = list;
    this.positionsCache.clear();
    super.submitList(list);
  }

  @Override
  public long getItemId(int position) {
    final PlaylistEntry entry = getItem(position);
    return entry != null ? entry.id : RecyclerView.NO_ID;
  }

  @NonNull
  @Override
  public EntryViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    final PlaylistEntryBinding binding = DataBindingUtil.inflate(inflater, R.layout.playlist_entry, parent, false);
    return new EntryViewHolder(binding);
  }

  @Override
  public void onBindViewHolder(@NonNull final EntryViewHolder holder, int position) {
    final PlaylistEntry entry = getItem(position);
    holder.bind(entry, isPlaying && isNowPlaying(entry.id), isSelected(entry.id));
  }

  private boolean isNowPlaying(long id) {
    return nowPlaying != null && nowPlaying == id;
  }

  private boolean isSelected(long id) {
    return selection != null && selection.contains(id);
  }

  @Override
  public void onViewAttachedToWindow(@NonNull final EntryViewHolder holder) {
    holder.binding.playlistEntryState.setOnTouchListener(new View.OnTouchListener() {
      @Override
      public boolean onTouch(View v, MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
          return client.onDrag(holder);
        }
        return false;
      }
    });
  }

  @Override
  public void onViewDetachedFromWindow(@NonNull EntryViewHolder holder) {
    holder.binding.playlistEntryState.setOnTouchListener(null);
  }

  static class EntryViewHolder extends RecyclerView.ViewHolder {
    private final PlaylistEntryBinding binding;

    EntryViewHolder(PlaylistEntryBinding binding) {
      super(binding.getRoot());
      this.binding = binding;
    }

    final void bind(@NonNull PlaylistEntry entry, boolean isPlaying, boolean isSelected) {
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

  public static class KeyProvider extends ItemKeyProvider<Long> {

    private final PlaylistViewAdapter adapter;

    public KeyProvider(PlaylistViewAdapter adapter) {
      super(SCOPE_MAPPED);
      this.adapter = adapter;
    }

    @Nullable
    @Override
    public Long getKey(int position) {
      final PlaylistEntry entry = adapter.getItem(position);
      return entry != null ? entry.id : null;
    }

    @Override
    public int getPosition(@NonNull Long key) {
      final Integer pos = adapter.getPosition(key);
      return pos != null ? pos : RecyclerView.NO_POSITION;
    }
  }

  public static class DetailsLookup extends ItemDetailsLookup<Long> {

    private final RecyclerView listing;

    public DetailsLookup(RecyclerView view) {
      this.listing = view;
    }

    @Nullable
    @Override
    public ItemDetails getItemDetails(@NonNull MotionEvent e) {
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
