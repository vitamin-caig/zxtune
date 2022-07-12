/**
 * @file
 * @brief Playlist fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui.playlist;

import android.app.Activity;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.selection.ItemDetailsLookup;
import androidx.recyclerview.selection.OnItemActivatedListener;
import androidx.recyclerview.selection.Selection;
import androidx.recyclerview.selection.SelectionPredicates;
import androidx.recyclerview.selection.SelectionTracker;
import androidx.recyclerview.selection.StorageStrategy;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.device.media.MediaSessionModel;
import app.zxtune.playlist.ProviderClient;
import app.zxtune.ui.utils.SelectionUtils;

public class PlaylistFragment extends Fragment {

  private static final String TAG = PlaylistFragment.class.getName();

  private static final String LISTING_STATE_KEY = "listing_state";

  @Nullable
  private ProviderClient ctrl;
  @Nullable
  private RecyclerView listing;
  @Nullable
  private ItemTouchHelper touchHelper;
  @Nullable
  private View stub;
  @Nullable
  private SelectionTracker<Long> selectionTracker;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }

  @Override
  public void onAttach(Context ctx) {
    super.onAttach(ctx);

    ctrl = ProviderClient.create(ctx);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setHasOptionsMenu(true);
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
    super.onCreateOptionsMenu(menu, inflater);

    inflater.inflate(R.menu.playlist, menu);
    final MenuItem sortItem = menu.findItem(R.id.action_sort);
    final SubMenu sortMenuRoot = sortItem.getSubMenu();
    for (ProviderClient.SortBy sortBy : ProviderClient.SortBy.values()) {
      for (ProviderClient.SortOrder sortOrder : ProviderClient.SortOrder.values()) {
        try {
          final MenuItem item = sortMenuRoot.add(getMenuTitle(sortBy));
          final ProviderClient.SortBy by = sortBy;
          final ProviderClient.SortOrder order = sortOrder;
          item.setOnMenuItemClickListener(item1 -> {
            ctrl.sort(by, order);
            return true;
          });
          item.setIcon(getMenuIcon(sortOrder));
        } catch (Exception e) {
          Log.w(TAG, e, "onCreateOptionsMenu");
        }
      }
    }
  }

  private static int getMenuTitle(ProviderClient.SortBy by) throws Exception {
    if (ProviderClient.SortBy.title.equals(by)) {
      return R.string.information_title;
    } else if (ProviderClient.SortBy.author.equals(by)) {
      return R.string.information_author;
    } else if (ProviderClient.SortBy.duration.equals(by)) {
      return R.string.statistics_duration;//TODO: extract
    } else {
      throw new Exception("Invalid sort order");
    }
  }

  private static int getMenuIcon(ProviderClient.SortOrder order) throws Exception {
    if (ProviderClient.SortOrder.asc.equals(order)) {
      return android.R.drawable.arrow_up_float;
    } else if (ProviderClient.SortOrder.desc.equals(order)) {
      return android.R.drawable.arrow_down_float;
    } else {
      throw new Exception("Invalid sor order");
    }
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    return processMenu(item.getItemId(), selectionTracker.getSelection()) || super.onOptionsItemSelected(item);
  }

  @Override
  @Nullable
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.playlist, container, false) : null;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    listing = view.findViewById(R.id.playlist_content);
    listing.setHasFixedSize(true);

    final ViewAdapter adapter = new ViewAdapter(new AdapterClient());
    listing.setAdapter(adapter);

    selectionTracker = new SelectionTracker.Builder<>("playlist_selection",
        listing,
        new ViewAdapter.KeyProvider(adapter),
        new ViewAdapter.DetailsLookup(listing),
        StorageStrategy.createLongStorage())
        .withSelectionPredicate(SelectionPredicates.<Long>createSelectAnything())
        .withOnItemActivatedListener(new ItemActivatedListener())
        .build();
    adapter.setSelection(selectionTracker.getSelection());

    // another class for test
    final Activity activity = getActivity();
    if (activity instanceof AppCompatActivity) {
      SelectionUtils.install((AppCompatActivity) activity, selectionTracker,
          new SelectionClient());
    }

    if (savedInstanceState != null) {
      restoreState(savedInstanceState);
    }

    final TouchHelperCallback touchHelperCallback = new TouchHelperCallback();
    touchHelper = new ItemTouchHelper(touchHelperCallback);
    touchHelper.attachToRecyclerView(listing);

    stub = view.findViewById(R.id.playlist_stub);

    final Model playlistModel = Model.of(this);
    playlistModel.getItems().observe(getViewLifecycleOwner(), entries -> {
      if (touchHelperCallback.isDragging()) {
        return;
      }
      adapter.submitList(entries);
      if (entries.isEmpty()) {
        listing.setVisibility(View.GONE);
        stub.setVisibility(View.VISIBLE);
      } else {
        listing.setVisibility(View.VISIBLE);
        stub.setVisibility(View.GONE);
      }
    });
    final MediaSessionModel model = MediaSessionModel.of(getActivity());
    model.getState().observe(getViewLifecycleOwner(), state -> {
      final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
      adapter.setIsPlaying(isPlaying);
    });
    model.getMetadata().observe(getViewLifecycleOwner(), metadata -> {
      if (metadata != null) {
        final Uri uri = Uri.parse(metadata.getDescription().getMediaId());
        adapter.setNowPlaying(ProviderClient.findId(uri));
      }
    });
  }

  private void restoreState(Bundle savedInstanceState) {
    selectionTracker.onRestoreInstanceState(savedInstanceState);
    final Parcelable listingState = savedInstanceState.getParcelable(LISTING_STATE_KEY);
    if (listingState != null) {
      listing.getLayoutManager().onRestoreInstanceState(listingState);
    }
  }

  @Override
  public void onSaveInstanceState(Bundle outState) {
    selectionTracker.onSaveInstanceState(outState);
    outState.putParcelable(LISTING_STATE_KEY, listing.getLayoutManager().onSaveInstanceState());
  }

  private class ItemActivatedListener implements OnItemActivatedListener<Long> {
    @Override
    public boolean onItemActivated(ItemDetailsLookup.ItemDetails<Long> item, MotionEvent e) {
      return onItemClick(item.getSelectionKey());
    }
  }

  private class AdapterClient implements ViewAdapter.Client {
    @Override
    public boolean onDrag(RecyclerView.ViewHolder holder) {
      return onItemDrag(holder);
    }
  }

  private boolean onItemClick(@Nullable Long id) {
    final MediaSessionModel model = MediaSessionModel.of(getActivity());
    final MediaControllerCompat ctrl = model.getMediaController().getValue();
    if (ctrl != null && id != null) {
      final Uri toPlay = ProviderClient.createUri(id);
      ctrl.getTransportControls().playFromUri(toPlay, null);
      return true;
    }
    return false;
  }

  private boolean onItemDrag(RecyclerView.ViewHolder holder) {
    // Disable dragging in selection mode
    if (!selectionTracker.hasSelection()) {
      touchHelper.startDrag(holder);
      return true;
    } else {
      return false;
    }
  }

  // ArchivesService for selection
  private class SelectionClient implements SelectionUtils.Client<Long> {
    @Override
    public String getTitle(int count) {
      return getResources().getQuantityString(R.plurals.tracks,
          count, count);
    }

    @Override
    public List<Long> getAllItems() {
      final RecyclerView.Adapter adapter = listing.getAdapter();
      if (adapter == null) {
        return new ArrayList<>(0);
      }
      final int size = adapter.getItemCount();
      final ArrayList<Long> res = new ArrayList<>(size);
      for (int i = 0; i < size; ++i) {
        res.add(adapter.getItemId(i));
      }
      return res;
    }

    @Override
    public void fillMenu(MenuInflater inflater, Menu menu) {
      inflater.inflate(R.menu.playlist_items, menu);
    }

    @Override
    public boolean processMenu(int itemId, Selection<Long> selection) {
      return PlaylistFragment.this.processMenu(itemId, selection);
    }
  }

  private boolean processMenu(int itemId, Selection<Long> selection) {
    switch (itemId) {
      case R.id.action_clear:
        ctrl.deleteAll();
        break;
      case R.id.action_delete:
        ctrl.delete(convertSelection(selection));
        break;
      case R.id.action_save:
        savePlaylist(convertSelection(selection));
        break;
      case R.id.action_statistics:
        showStatistics(convertSelection(selection));
        break;
      default:
        return false;
    }
    return true;
  }

  @Nullable
  private static long[] convertSelection(@Nullable Selection<Long> selection) {
    if (selection == null || selection.size() == 0) {
      return null;
    }
    final long[] result = new long[selection.size()];
    final Iterator<Long> iter = selection.iterator();
    for (int idx = 0; idx < result.length; ++idx) {
      result[idx] = iter.next();
    }
    return result;
  }

  private void savePlaylist(@Nullable long[] ids) {
    PlaylistSaveFragment.createInstance(ids).show(getFragmentManager(), "save");
  }

  private void showStatistics(@Nullable long[] ids) {
    PlaylistStatisticsFragment.createInstance(ids).show(getFragmentManager(), "statistics");
  }

  private class TouchHelperCallback extends ItemTouchHelper.SimpleCallback {

    boolean isActive = false;
    long draggedItem = -1;
    int dragDelta = 0;

    TouchHelperCallback() {
      super(ItemTouchHelper.UP | ItemTouchHelper.DOWN, 0);
    }

    final boolean isDragging() {
      return isActive;
    }

    @Override
    public boolean isLongPressDragEnabled() {
      return false;
    }

    @Override
    public boolean isItemViewSwipeEnabled() {
      return false;
    }

    @Override
    public void onSelectedChanged(@Nullable RecyclerView.ViewHolder viewHolder, int actionState) {
      if (viewHolder != null && actionState != ItemTouchHelper.ACTION_STATE_IDLE) {
        isActive = true;
        viewHolder.itemView.setActivated(true);
      }
      super.onSelectedChanged(viewHolder, actionState);
    }

    @Override
    public void clearView(RecyclerView view, RecyclerView.ViewHolder viewHolder) {
      super.clearView(view, viewHolder);
      isActive = false;
      viewHolder.itemView.setActivated(false);

      if (draggedItem != -1 && dragDelta != 0) {
        ctrl.move(draggedItem, dragDelta);
      }
      draggedItem = -1;
      dragDelta = 0;
    }

    @Override
    public boolean onMove(RecyclerView recyclerView, RecyclerView.ViewHolder source,
                          RecyclerView.ViewHolder target) {
      final int srcPos = source.getAdapterPosition();
      final int tgtPos = target.getAdapterPosition();
      if (draggedItem == -1) {
        draggedItem = source.getItemId();
      }
      dragDelta += tgtPos - srcPos;
      final ViewAdapter adapter = ((ViewAdapter) recyclerView.getAdapter());
      if (adapter != null) {
        adapter.onItemMove(srcPos, tgtPos);
      }
      return true;
    }

    @Override
    public void onSwiped(RecyclerView.ViewHolder viewHolder, int direction) {
    }
  }
}
