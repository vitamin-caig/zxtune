/**
 * @file
 * @brief Playlist fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProviders;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.support.v4.media.session.MediaControllerCompat;
import android.view.ActionMode;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.Log;
import app.zxtune.Preferences;
import app.zxtune.R;
import app.zxtune.models.MediaSessionModel;
import app.zxtune.playlist.ProviderClient;
import app.zxtune.ui.utils.ListViewTools;

public class PlaylistFragment extends Fragment {

  private static final String TAG = PlaylistFragment.class.getName();
  private ProviderClient ctrl;
  private PlaylistState state;
  private PlaylistView listing;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }

  @Override
  public void onAttach(Context ctx) {
    super.onAttach(ctx);

    ctrl = new ProviderClient(ctx);
    state = new PlaylistState(Preferences.getDefaultSharedPreferences(ctx));
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
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
          item.setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
              ctrl.sort(by, order);
              return true;
            }
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
    switch (item.getItemId()) {
      case R.id.action_clear:
        ctrl.deleteAll();
        break;
      case R.id.action_save:
        savePlaylist(null);
        break;
      case R.id.action_statistics:
        showStatistics(null);
        break;
      default:
        return super.onOptionsItemSelected(item);
    }
    return true;
  }

  private void savePlaylist(@Nullable long[] ids) {
    PlaylistSaveFragment.createInstance(ids).show(getFragmentManager(), "save");
  }

  private void showStatistics(@Nullable long[] ids) {
    PlaylistStatisticsFragment.createInstance(ids).show(getFragmentManager(), "statistics");
  }

  @Override
  @Nullable
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.playlist, container, false) : null;
  }

  @Override
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    listing = view.findViewById(R.id.playlist_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setEmptyView(view.findViewById(R.id.playlist_stub));
    listing.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE_MODAL);
    listing.setMultiChoiceModeListener(new MultiChoiceModeListener());
    setEmptyText(R.string.starting);

    if (savedInstanceState == null) {
      Log.d(TAG, "Loading persistent state");
      ListViewTools.storeViewPosition(listing, state.getCurrentViewPosition());
    }
    listing.setRemoveListener(new PlaylistView.RemoveListener() {
      @Override
      public void remove(int which) {
        final long[] id = {listing.getItemIdAtPosition(which)};
        ctrl.delete(id);
      }
    });
    listing.setDropListener(new PlaylistView.DropListener() {
      @Override
      public void drop(int from, int to) {
        if (from != to) {
          //TODO: perform in separate thread
          final long id = listing.getItemIdAtPosition(from);
          ctrl.move(id, to - from);
        }
      }
    });
  }

  @Override
  public void onStart() {
    super.onStart();

    loadListing();
  }

  @Override
  public void onStop() {
    super.onStop();

    Log.d(TAG, "Saving persistent state");
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }

  private void loadListing() {
    listing.load(getLoaderManager());
    setEmptyText(R.string.playlist_empty);
  }

  private void setEmptyText(int res) {
    ((TextView) listing.getEmptyView()).setText(res);
  }

  private class OnItemClickListener implements PlaylistView.OnItemClickListener {

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
      final MediaSessionModel model = ViewModelProviders.of(getActivity()).get(MediaSessionModel.class);
      final MediaControllerCompat ctrl = model.getMediaController().getValue();
      if (ctrl != null) {
        final Uri toPlay = ProviderClient.createUri(id);
        ctrl.getTransportControls().playFromUri(toPlay, null);
      }
    }
  }

  private String getActionModeTitle() {
    final int count = listing.getCheckedItemCount();
    return getResources().getQuantityString(R.plurals.tracks, count, count);
  }

  private class MultiChoiceModeListener implements ListView.MultiChoiceModeListener {

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.selection, menu);
      inflater.inflate(R.menu.playlist_items, menu);
      mode.setTitle(getActionModeTitle());
      return true;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
      if (ListViewTools.processActionItemClick(listing, item.getItemId())) {
        return true;
      } else {
        switch (item.getItemId()) {
          case R.id.action_delete:
            ctrl.delete(listing.getCheckedItemIds());
            break;
          case R.id.action_save:
            savePlaylist(listing.getCheckedItemIds());
            break;
          case R.id.action_statistics:
            showStatistics(listing.getCheckedItemIds());
            break;
          default:
            return false;
        }
        mode.finish();
        return true;
      }
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
    }

    @Override
    public void onItemCheckedStateChanged(ActionMode mode, int position, long id, boolean checked) {
      mode.setTitle(getActionModeTitle());
    }
  }
}
