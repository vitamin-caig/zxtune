/**
 * @file
 * @brief Vfs browser fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui.browser;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.media.session.MediaControllerCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SearchView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;
import androidx.recyclerview.selection.ItemDetailsLookup;
import androidx.recyclerview.selection.OnItemActivatedListener;
import androidx.recyclerview.selection.Selection;
import androidx.recyclerview.selection.SelectionPredicates;
import androidx.recyclerview.selection.SelectionTracker;
import androidx.recyclerview.selection.StorageStrategy;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.ui.utils.SelectionUtils;

public class BrowserFragment extends Fragment {

  //private static final String TAG = BrowserFragment.class.getName();
  private static final String SEARCH_QUERY_KEY = "search_query";

  @Nullable
  private RecyclerView listing;
  @Nullable
  private SearchView search;
  @Nullable
  private State stateStorage;
  @Nullable
  private SelectionTracker<Uri> selectionTracker;

  public static BrowserFragment createInstance() {
    return new BrowserFragment();
  }

  @Override
  public void onAttach(Context ctx) {
    super.onAttach(ctx);

    this.stateStorage = new State(this);
  }

  @Override
  @Nullable
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.browser, container, false) : null;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    final Model model = Model.of(this);
    setupRootsView(view);
    setupBreadcrumbs(model, view);
    listing = setupListing(model, view);
    search = setupSearchView(model, view);

    if (model.getState().getValue() == null) {
      browse(stateStorage.getCurrentPath());
    }
  }

  private void setupRootsView(View view) {
    final View roots = view.findViewById(R.id.browser_roots);
    roots.setOnClickListener(v -> browse(Uri.EMPTY));
  }

  private void setupBreadcrumbs(Model model, View view) {
    final RecyclerView listing = view.findViewById(R.id.browser_breadcrumb);
    final BreadcrumbsViewAdapter adapter = new BreadcrumbsViewAdapter();
    listing.setAdapter(adapter);
    model.getState().observe(this, state -> {
      adapter.submitList(state.breadcrumbs);
      listing.smoothScrollToPosition(state.breadcrumbs.size());
    });
    final View.OnClickListener onClick = v -> {
      final int pos = listing.getChildAdapterPosition(v);
      final BreadcrumbsEntry entry = adapter.getCurrentList().get(pos);
      browse(entry.getUri());
    };
    listing.addOnChildAttachStateChangeListener(new RecyclerView.OnChildAttachStateChangeListener() {
      @Override
      public void onChildViewAttachedToWindow(View view) {
        view.setOnClickListener(onClick);
      }

      @Override
      public void onChildViewDetachedFromWindow(View view) {
        view.setOnClickListener(null);
      }
    });
  }

  private RecyclerView setupListing(Model model, View view) {
    final RecyclerView listing = view.findViewById(R.id.browser_content);
    listing.setHasFixedSize(true);

    final ListingViewAdapter adapter = new ListingViewAdapter();
    listing.setAdapter(adapter);

    selectionTracker = new SelectionTracker.Builder<>("browser_selection",
        listing, new ListingViewAdapter.KeyProvider(adapter),
        new ListingViewAdapter.DetailsLookup(listing),
        StorageStrategy.createParcelableStorage(Uri.class))
        .withSelectionPredicate(SelectionPredicates.<Uri>createSelectAnything())
        .withOnItemActivatedListener(new ItemActivatedListener())
        .build();
    adapter.setSelection(selectionTracker.getSelection());

    SelectionUtils.install((AppCompatActivity) getActivity(), selectionTracker,
        new SelectionClient(adapter));

    final TextView stub = view.findViewById(R.id.browser_stub);
    model.getState().observe(this, state -> {
      storeCurrentViewPosition(listing);
      adapter.submitList(state.entries, () -> {
        stateStorage.setCurrentPath(state.uri);
        restoreCurrentViewPosition(listing);
      });
      if (state.entries.isEmpty()) {
        listing.setVisibility(View.GONE);
        stub.setVisibility(View.VISIBLE);
      } else {
        listing.setVisibility(View.VISIBLE);
        stub.setVisibility(View.GONE);
      }
    });
    final ProgressBar progress = view.findViewById(R.id.browser_loading);
    model.getProgress().observe(this, prg -> {
      if (prg == null) {
        progress.setIndeterminate(false);
        progress.setProgress(0);
      } else if (prg == -1) {
        progress.setIndeterminate(true);
      } else {
        progress.setIndeterminate(false);
        progress.setProgress(prg);
      }
    });
    model.setClient(new Model.Client() {
      @Override
      public void onFileBrowse(Uri uri) {
        final MediaControllerCompat ctrl = getController();
        if (ctrl != null) {
          ctrl.getTransportControls().playFromUri(uri, null);
        }
      }

      @Override
      public void onError(final String msg) {
        listing.post(() -> Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show());
      }
    });
    getLifecycle().addObserver(new LifecycleObserver() {
      @SuppressWarnings("unused")
      @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
      void onStop() {
        storeCurrentViewPosition(listing);
        getLifecycle().removeObserver(this);
      }
    });
    return listing;
  }

  private void storeCurrentViewPosition(RecyclerView listing) {
    final LinearLayoutManager layout = ((LinearLayoutManager) listing.getLayoutManager());
    final int pos = layout != null ? layout.findFirstVisibleItemPosition() : -1;
    if (pos != -1) {
      stateStorage.setCurrentViewPosition(pos);
    }
  }

  private void restoreCurrentViewPosition(RecyclerView listing) {
    final LinearLayoutManager layout = ((LinearLayoutManager) listing.getLayoutManager());
    if (layout != null) {
      layout.scrollToPositionWithOffset(stateStorage.getCurrentViewPosition()
          , 0);
    }
  }

  private SearchView setupSearchView(final Model model, View view) {
    final SearchView search = view.findViewById(R.id.browser_search);

    search.setOnSearchClickListener(view1 -> {
      final LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) search.getLayoutParams();
      params.width = LayoutParams.MATCH_PARENT;
      search.setLayoutParams(params);
    });
    search.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
      @Override
      public boolean onQueryTextSubmit(String query) {
        model.search(query);
        search.clearFocus();
        return true;
      }

      @Override
      public boolean onQueryTextChange(String query) {
        ((ListingViewAdapter) listing.getAdapter()).setFilter(query);
        return true;
      }
    });
    search.setOnCloseListener(() -> {
      final LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) search.getLayoutParams();
      params.width = LayoutParams.WRAP_CONTENT;
      search.setLayoutParams(params);
      search.post(() -> {
        search.clearFocus();
        model.reload();
      });
      return false;
    });
    search.setOnFocusChangeListener((v, hasFocus) -> {
      if (!search.isIconified() && 0 == search.getQuery().length()) {
        search.setIconified(true);
      }
    });
    return search;
  }

  private class ItemActivatedListener implements OnItemActivatedListener<Uri> {
    @Override
    public boolean onItemActivated(ItemDetailsLookup.ItemDetails<Uri> item,
                                   MotionEvent e) {
      final Uri uri = item.getSelectionKey();
      if (uri != null) {
        browse(uri);
      }
      return true;
    }
  }

  // ArchivesService for selection
  private class SelectionClient implements SelectionUtils.Client<Uri> {

    private final ListingViewAdapter adapter;

    SelectionClient(ListingViewAdapter adapter) {
      this.adapter = adapter;
    }

    @Override
    public String getTitle(int count) {
      return getResources().getQuantityString(R.plurals.items,
          count, count);
    }

    @Override
    public List<Uri> getAllItems() {
      final int size = adapter.getItemCount();
      final ArrayList<Uri> res = new ArrayList<>(size);
      for (int i = 0; i < size; ++i) {
        res.add(adapter.getItemUri(i));
      }
      return res;
    }

    @Override
    public void fillMenu(MenuInflater inflater, Menu menu) {
      inflater.inflate(R.menu.browser, menu);
    }

    @Override
    public boolean processMenu(int itemId, Selection<Uri> selection) {
      switch (itemId) {
        case R.id.action_add:
          addToPlaylist(selection);
          break;
        default:
          return false;
      }
      return true;
    }

    private void addToPlaylist(Selection<Uri> selection) {
      //TODO: rework for PlaylistControl usage as a local iface
      final MediaControllerCompat ctrl = getController();
      if (ctrl != null) {
        final Bundle params = new Bundle();
        params.putParcelableArray("uris", convertSelection(selection));
        ctrl.getTransportControls().sendCustomAction(MainService.CUSTOM_ACTION_ADD, params);
      }
    }
  }

  private static Uri[] convertSelection(Selection<Uri> selection) {
    final Uri[] result = new Uri[selection.size()];
    final Iterator<Uri> iter = selection.iterator();
    for (int idx = 0; idx < result.length; ++idx) {
      result[idx] = iter.next();
    }
    return result;
  }


  @Override
  public void onSaveInstanceState(Bundle state) {
    super.onSaveInstanceState(state);
    selectionTracker.onSaveInstanceState(state);
    if (!search.isIconified()) {
      final String query = search.getQuery().toString();
      state.putString(SEARCH_QUERY_KEY, query);
    }
  }

  @Override
  public void onViewStateRestored(@Nullable Bundle state) {
    super.onViewStateRestored(state);
    selectionTracker.onRestoreInstanceState(state);
    final String query = state != null ? state.getString(SEARCH_QUERY_KEY) : null;
    if (!TextUtils.isEmpty(query)) {
      search.post(() -> {
        search.setIconified(false);
        search.setQuery(query, false);
      });
    }
  }

  private void browse(Uri uri) {
    final Model model = Model.of(this);
    model.browse(uri);
  }

  public final void moveUp() {
    if (!search.isIconified()) {
      search.setIconified(true);
    } else {
      browseParent();
    }
  }

  private void browseParent() {
    final Model model = Model.of(this);
    model.browseParent();
  }

  @Nullable
  private MediaControllerCompat getController() {
    final FragmentActivity activity = getActivity();
    return activity != null
        ? MediaControllerCompat.getMediaController(activity)
        : null;
  }
}
