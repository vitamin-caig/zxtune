/**
 * @file
 * @brief Vfs browser fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import android.support.v4.media.session.MediaControllerCompat;
import android.util.SparseBooleanArray;
import android.view.ActionMode;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.SearchView;

import app.zxtune.Log;
import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.ui.browser.BreadCrumbsView;
import app.zxtune.ui.browser.BrowserController;
import app.zxtune.ui.utils.ListViewTools;
import app.zxtune.ui.utils.UiUtils;

public class BrowserFragment extends Fragment {

  private static final String TAG = BrowserFragment.class.getName();
  private static final String SEARCH_QUERY_KEY = "search_query";
  private static final String SEARCH_FOCUSED_KEY = "search_focused";

  private BrowserController controller;
  private View sources;
  private SearchView search;
  private ListView listing;

  public static BrowserFragment createInstance() {
    return new BrowserFragment();
  }

  @Override
  public void onAttach(Context ctx) {
    super.onAttach(ctx);

    this.controller = new BrowserController(this);
  }

  @Override
  @Nullable
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.browser, container, false) : null;
  }

  @Override
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    sources = view.findViewById(R.id.browser_sources);
    search = setupSearchView(view);
    setupRootsView(view);
    final BreadCrumbsView position = setupPositionView(view);
    final ProgressBar progress = view.findViewById(R.id.browser_loading);
    listing = setupListing(view);

    controller.setViews(position, progress, listing);

    controller.loadState();
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();

    Log.d(TAG, "Saving persistent state");
    controller.storeCurrentViewPosition();
    controller.resetViews();
  }

  private ListView setupListing(View view) {
    final ListView listing = view.findViewById(R.id.browser_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setEmptyView(view.findViewById(R.id.browser_stub));
    listing.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE_MODAL);
    listing.setMultiChoiceModeListener(new MultiChoiceModeListener());
    return listing;
  }

  private BreadCrumbsView setupPositionView(View view) {
    final BreadCrumbsView position = view.findViewById(R.id.browser_breadcrumb);
    position.setDirSelectionListener(new BreadCrumbsView.DirSelectionListener() {
      @Override
      public void onDirSelection(VfsDir dir) {
        controller.browseDir(dir);
      }
    });
    return position;
  }

  private void setupRootsView(View view) {
    final View roots = view.findViewById(R.id.browser_roots);
    roots.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        controller.browseRoot();
      }
    });
  }

  private SearchView setupSearchView(View view) {
    final SearchView search = view.findViewById(R.id.browser_search);

    search.setOnSearchClickListener(new SearchView.OnClickListener() {
      @Override
      public void onClick(View view) {
        final LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) search.getLayoutParams();
        params.width = LayoutParams.MATCH_PARENT;
        search.setLayoutParams(params);
      }
    });
    search.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
      @Override
      public boolean onQueryTextSubmit(String query) {
        controller.search(query);
        search.clearFocus();
        return true;
      }

      @Override
      public boolean onQueryTextChange(String query) {
        return false;
      }
    });
    search.setOnCloseListener(new SearchView.OnCloseListener() {
      @Override
      public boolean onClose() {
        final LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) search.getLayoutParams();
        params.width = LayoutParams.WRAP_CONTENT;
        search.setLayoutParams(params);
        search.post(new Runnable() {
          @Override
          public void run() {
            search.clearFocus();
            if (controller.isInSearch()) {
              controller.loadCurrentDir();
            }
          }
        });
        return false;
      }
    });
    search.setOnFocusChangeListener(new View.OnFocusChangeListener() {
      @Override
      public void onFocusChange(View v, boolean hasFocus) {
        if (!search.isIconified() && 0 == search.getQuery().length()) {
          search.setIconified(true);
        }
      }
    });
    return search;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle state) {
    super.onSaveInstanceState(state);
    if (!search.isIconified()) {
      final String query = search.getQuery().toString();
      state.putString(SEARCH_QUERY_KEY, query);
      state.putBoolean(SEARCH_FOCUSED_KEY, search.hasFocus());
    }
  }

  @Override
  public void onViewStateRestored(Bundle state) {
    super.onViewStateRestored(state);
    if (state == null || !state.containsKey(SEARCH_QUERY_KEY)) {
      return;
    }
    final String query = state.getString(SEARCH_QUERY_KEY);
    final boolean isFocused = state.getBoolean(SEARCH_FOCUSED_KEY);
    search.post(new Runnable() {
      @Override
      public void run() {
        search.setIconified(false);
        search.setQuery(query, false);
        if (!isFocused) {
          search.clearFocus();
        }
      }
    });
  }

  public final void moveUp() {
    if (!search.isIconified()) {
      search.setIconified(true);
    } else {
      controller.moveToParent();
    }
  }

  private String getActionModeTitle() {
    final int count = listing.getCheckedItemCount();
    return getResources().getQuantityString(R.plurals.items, count, count);
  }

  @Nullable
  private MediaControllerCompat getController() {
    final FragmentActivity activity = getActivity();
    return activity != null
               ? MediaControllerCompat.getMediaController(activity)
               : null;
  }

  class OnItemClickListener implements AdapterView.OnItemClickListener {

    @Override
    public void onItemClick(AdapterView<?> parent, View view, final int position, long id) {
      final Runnable playCmd = new Runnable() {
        final Uri toPlay = getUriFrom(position);

        @Override
        public void run() {
          final MediaControllerCompat ctrl = getController();
          if (ctrl != null) {
            ctrl.getTransportControls().playFromUri(toPlay, null);
          }
        }
      };
      final Object obj = parent.getItemAtPosition(position);
      if (obj instanceof VfsDir) {
        final Object feed = ((VfsDir) obj).getExtension(VfsExtensions.FEED);
        if (feed != null) {
          playCmd.run();
        } else {
          controller.browseDir((VfsDir) obj);
        }
      } else if (obj instanceof VfsFile) {
        if (controller.isInSearch()) {
          playCmd.run();
        } else {
          controller.browseArchive((VfsFile) obj, playCmd);
        }
      }
    }

    private Uri getUriFrom(int position) {
      final ListAdapter adapter = listing.getAdapter();
      final VfsObject obj = (VfsObject) adapter.getItem(position);
      return obj.getUri();
    }
  }

  private class MultiChoiceModeListener implements ListView.MultiChoiceModeListener {

    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
      UiUtils.setViewEnabled(sources, false);
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.selection, menu);
      inflater.inflate(R.menu.browser, menu);
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
          case R.id.action_add:
            addSelectedToPlaylist();
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
      UiUtils.setViewEnabled(sources, true);
    }

    @Override
    public void onItemCheckedStateChanged(ActionMode mode, int position, long id, boolean checked) {
      mode.setTitle(getActionModeTitle());
    }

    private void addSelectedToPlaylist() {
      //TODO: rework for PlaylistControl usage as a local iface
      final MediaControllerCompat ctrl = getController();
      if (ctrl != null) {
        final Uri[] items = getSelectedItemsUris();
        final Bundle params = new Bundle();
        params.putParcelableArray("uris", items);
        ctrl.getTransportControls().sendCustomAction(MainService.CUSTOM_ACTION_ADD, params);
      }
    }

    private Uri[] getSelectedItemsUris() {
      final Uri[] result = new Uri[listing.getCheckedItemCount()];
      final SparseBooleanArray selected = listing.getCheckedItemPositions();
      final ListAdapter adapter = listing.getAdapter();
      int pos = 0;
      for (int i = 0, lim = selected.size(); i != lim; ++i) {
        if (selected.valueAt(i)) {
          final VfsObject obj = (VfsObject) adapter.getItem(selected.keyAt(i));
          result[pos++] = obj.getUri();
        }
      }
      return result;
    }
  }
}
