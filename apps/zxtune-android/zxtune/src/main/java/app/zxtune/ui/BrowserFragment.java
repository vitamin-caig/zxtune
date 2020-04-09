/**
 * @file
 * @brief Vfs browser fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

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

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.Observer;
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
import app.zxtune.ui.browser.BreadcrumbsViewAdapter;
import app.zxtune.ui.browser.BrowserEntrySimple;
import app.zxtune.ui.browser.BrowserModel;
import app.zxtune.ui.browser.BrowserState;
import app.zxtune.ui.browser.ListingViewAdapter;
import app.zxtune.ui.utils.SelectionUtils;

public class BrowserFragment extends Fragment {

  //private static final String TAG = BrowserFragment.class.getName();
  private static final String SEARCH_QUERY_KEY = "search_query";

  private SearchView search;
  private BrowserState stateStorage;
  private SelectionTracker<Uri> selectionTracker;

  public static BrowserFragment createInstance() {
    return new BrowserFragment();
  }

  @Override
  public void onAttach(@NonNull Context ctx) {
    super.onAttach(ctx);

    this.stateStorage = new BrowserState(this);
  }

  @Override
  @Nullable
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.browser, container, false) : null;
  }

  @Override
  public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    final BrowserModel model = BrowserModel.of(this);
    setupRootsView(view);
    setupBreadcrumbs(model, view);
    setupListing(model, view);
    search = setupSearchView(model, view);

    if (model.getState().getValue() == null) {
      browse(stateStorage.getCurrentPath());
    }
  }

  private void setupRootsView(@NonNull View view) {
    final View roots = view.findViewById(R.id.browser_roots);
    roots.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        browse(Uri.EMPTY);
      }
    });
  }

  private void setupBreadcrumbs(@NonNull BrowserModel model, @NonNull View view) {
    final RecyclerView listing = view.findViewById(R.id.browser_breadcrumb);
    final BreadcrumbsViewAdapter adapter = new BreadcrumbsViewAdapter();
    listing.setAdapter(adapter);
    model.getState().observe(this, new Observer<BrowserModel.State>() {
      @Override
      public void onChanged(BrowserModel.State state) {
        adapter.submitList(state.breadcrumbs);
        listing.smoothScrollToPosition(state.breadcrumbs.size());
      }
    });
    final View.OnClickListener onClick = new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        final int pos = listing.getChildAdapterPosition(v);
        final BrowserEntrySimple entry = adapter.getCurrentList().get(pos);
        browse(entry.uri);
      }
    };
    listing.addOnChildAttachStateChangeListener(new RecyclerView.OnChildAttachStateChangeListener() {
      @Override
      public void onChildViewAttachedToWindow(@NonNull View view) {
        view.setOnClickListener(onClick);
      }

      @Override
      public void onChildViewDetachedFromWindow(@NonNull View view) {
        view.setOnClickListener(null);
      }
    });
  }

  private void setupListing(@NonNull BrowserModel model, @NonNull View view) {
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
    model.getState().observe(this, new Observer<BrowserModel.State>() {
      @Override
      public void onChanged(final BrowserModel.State state) {
        storeCurrentViewPosition(listing);
        adapter.submitList(state.entries, new Runnable() {
          @Override
          public void run() {
            stateStorage.setCurrentPath(state.uri);
            restoreCurrentViewPosition(listing);
          }
        });
        if (state.entries.isEmpty()) {
          listing.setVisibility(View.GONE);
          stub.setVisibility(View.VISIBLE);
        } else {
          listing.setVisibility(View.VISIBLE);
          stub.setVisibility(View.GONE);
        }
      }
    });
    final ProgressBar progress = view.findViewById(R.id.browser_loading);
    model.getProgress().observe(this, new Observer<Integer>() {
      @Override
      public void onChanged(Integer prg) {
        if (prg == null) {
          progress.setIndeterminate(false);
          progress.setProgress(0);
        } else if (prg == -1) {
          progress.setIndeterminate(true);
        } else {
          progress.setIndeterminate(false);
          progress.setProgress(prg);
        }
      }
    });
    model.setClient(new BrowserModel.Client() {
      @Override
      public void onFileBrowse(Uri uri) {
        final MediaControllerCompat ctrl = getController();
        if (ctrl != null) {
          ctrl.getTransportControls().playFromUri(uri, null);
        }
      }

      @Override
      public void onError(final String msg) {
        listing.post(new Runnable() {
          @Override
          public void run() {
            Toast.makeText(getActivity(), msg, Toast.LENGTH_LONG).show();
          }
        });
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

  private SearchView setupSearchView(@NonNull final BrowserModel model, @NonNull View view) {
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
        model.search(query);
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
            model.reload();
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

  private class ItemActivatedListener implements OnItemActivatedListener<Uri> {
    @Override
    public boolean onItemActivated(@NonNull ItemDetailsLookup.ItemDetails<Uri> item,
                                   @NonNull MotionEvent e) {
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

    @NonNull
    @Override
    public String getTitle(int count) {
      return getResources().getQuantityString(R.plurals.items,
          count, count);
    }

    @NonNull
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
          break;
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
  public void onSaveInstanceState(@NonNull Bundle state) {
    super.onSaveInstanceState(state);
    selectionTracker.onSaveInstanceState(state);
    if (!search.isIconified()) {
      final String query = search.getQuery().toString();
      state.putString(SEARCH_QUERY_KEY, query);
    }
  }

  @Override
  public void onViewStateRestored(Bundle state) {
    super.onViewStateRestored(state);
    selectionTracker.onRestoreInstanceState(state);
    final String query = state != null ? state.getString(SEARCH_QUERY_KEY) : null;
    if (!TextUtils.isEmpty(query)) {
      search.post(new Runnable() {
        @Override
        public void run() {
          search.setIconified(false);
          search.setQuery(query, false);
        }
      });
    }
  }

  private void browse(@NonNull Uri uri) {
    final BrowserModel model = BrowserModel.of(this);
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
    final BrowserModel model = BrowserModel.of(this);
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
