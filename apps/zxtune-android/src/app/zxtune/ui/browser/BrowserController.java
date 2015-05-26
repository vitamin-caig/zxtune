/**
 * 
 * @file
 *
 * @brief Browser business logic storage
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import java.io.IOException;

import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.View;
import android.widget.BaseAdapter;
import android.widget.ProgressBar;
import app.zxtune.MainApplication;
import app.zxtune.Preferences;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;

public class BrowserController {
  
  private final static String TAG = BrowserController.class.getName();
  private final static int LOADER_ID = BrowserController.class.hashCode();
  
  private final LoaderManager loaderManager;
  private final BrowserState state;
  private BreadCrumbsView position;
  private ProgressBar progress;
  private BrowserView listing;

  public BrowserController(Fragment fragment) {
    this.loaderManager = fragment.getLoaderManager();
    this.state = new BrowserState(Preferences.getDefaultSharedPreferences(fragment.getActivity()));
  }
  
  public final void setViews(BreadCrumbsView position, ProgressBar progress, BrowserView listing) {
    this.position = position;
    this.progress = progress;
    this.listing = listing;
  }
  
  public final void resetViews() {
    final Loader<?> loader = loaderManager.getLoader(LOADER_ID); 
    if (loader instanceof SearchingLoader) {
      SearchingLoaderCallback.detachLoader((SearchingLoader) loader);
    } else if (loader instanceof ListingLoader) {
      ListingLoaderCallback.detachLoader((ListingLoader) loader);
    }
  }
  
  public final void loadState() {
    if (!reloadCurrentState()) {
      loadCurrentDir();
    }
  }
  
  public final void search(String query) {
    try {
      final Uri currentPath = state.getCurrentPath();
      final VfsDir currentDir = (VfsDir) Vfs.resolve(currentPath);
      loaderManager.destroyLoader(LOADER_ID);
      final LoaderManager.LoaderCallbacks<?> cb = SearchingLoaderCallback.create(this, currentDir, query);
      loaderManager.initLoader(LOADER_ID, null, cb).forceLoad();
    } catch (Exception e) {
      listing.showError(e);
    }
  }
  
  public final boolean isInSearch() {
    final Loader<?> loader = loaderManager.getLoader(LOADER_ID); 
    return loader instanceof SearchingLoader;
  }
  
  public final void setCurrentDir(VfsDir dir) {
    storeCurrentViewPosition();
    state.setCurrentPath(dir.getUri());
    setDirectory(dir);
  }
  
  public final void moveToParent() {
    try {
      final VfsDir root = Vfs.getRoot();
      final VfsDir curDir = (VfsDir) Vfs.resolve(state.getCurrentPath());
      if (curDir != root) {
        final VfsDir parent = curDir != null ? (VfsDir) curDir.getParent() : null;
        setCurrentDir(parent != null ? parent : root);
      }
    } catch (IOException e) {
      listing.showError(e);
    }
  }
  
  public final void storeCurrentViewPosition() {
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }

  private boolean reloadCurrentState() {
    final Loader<?> loader = loaderManager.getLoader(LOADER_ID); 
    if (loader == null) {
      Log.d(TAG, "Expired loader");
      return false;
    }
    if (loader instanceof SearchingLoader) {
      loaderManager.initLoader(LOADER_ID, null, SearchingLoaderCallback.create(this, (SearchingLoader) loader));
    } else {
      loaderManager.initLoader(LOADER_ID, null, ListingLoaderCallback.create(this, (ListingLoader) loader));
    }
    return true;
  }
  
  public final void loadCurrentDir() {
    try {
      final Uri currentPath = state.getCurrentPath();
      final VfsDir currentDir = (VfsDir) Vfs.resolve(currentPath);
      setDirectory(currentDir);
    } catch (Exception e) {
      listing.showError(e);
    }
  }
  
  private void setDirectory(VfsDir dir) {
    position.setDir(dir);
    loadListing(dir, state.getCurrentViewPosition());
  }
  
  //Required to call forceLoad due to bug in support library.
  //Some methods on callback does not called... 
  private void loadListing(VfsDir dir, int viewPosition) {
    loaderManager.destroyLoader(LOADER_ID);
    listing.storeViewPosition(viewPosition);
    final LoaderManager.LoaderCallbacks<?> cb = ListingLoaderCallback.create(this, dir);
    loaderManager.initLoader(LOADER_ID, null, cb).forceLoad();
  }
  
  private static class ListingLoaderCallback implements LoaderManager.LoaderCallbacks<Object>, ListingLoader.Callback {
    
    private final VfsDir dir;
    private final Handler handler;
    private BrowserController control;
    
    private ListingLoaderCallback(VfsDir dir) {
      this.dir = dir;
      this.handler = new Handler();
    }
    
    static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, VfsDir dir) {
      final ListingLoaderCallback cb = new ListingLoaderCallback(dir);
      cb.control = ctrl;
      ctrl.listing.setModel(null);
      ctrl.listingStarted();
      return cb;
    }
    
    static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, ListingLoader loader) {
      final ListingLoaderCallback cb = (ListingLoaderCallback) loader.getCallback();
      synchronized (cb) {
        cb.control = ctrl;
      }
      if (loader.isStarted()) {
        ctrl.listingStarted();
      }
      ctrl.position.setDir(cb.dir);
      return cb;
    }
    
    static void detachLoader(ListingLoader loader) {
      final ListingLoaderCallback cb = (ListingLoaderCallback) loader.getCallback();
      synchronized (cb) {
        cb.control = null;
        cb.handler.removeCallbacksAndMessages(null);
      }
    }

    @Override
    public Loader<Object> onCreateLoader(int id, Bundle params) {
      return new ListingLoader(MainApplication.getInstance(), dir, this);
    }

    @Override
    public synchronized void onLoadFinished(Loader<Object> loader, Object result) {
      if (control == null) {
        return;
      } else if (result instanceof BrowserViewModel) {
        control.loadingFinished();
        control.listing.setModel((BrowserViewModel) result);
        control.listing.useStoredViewPosition();
      } else {
        control.loadingFailed((Exception) result);
      }
    }

    @Override
    public synchronized void onLoaderReset(Loader<Object> loader) {
      if (control != null) {
        control.loadingFinished();
      }
    }

    @Override
    public synchronized void onProgressInit(int total) {
      if (control != null) {
        control.progress.setProgress(0);
        control.progress.setMax(total);
        control.progress.setIndeterminate(false);
      }
    }

    @Override
    public void onProgressUpdate(int current) {
      if (0 == current % 5) {
        postProgress(current);
      }
    }
    
    private synchronized void postProgress(final int current) {
      if (control != null) {
        final ProgressBar progress = control.progress;
        handler.post(new Runnable() {
          @Override
          public void run() {
            progress.setProgress(current);
          }
        });
      }
    }
  }
  
  private static class SearchingLoaderCallback implements LoaderManager.LoaderCallbacks<Void>, SearchingLoader.Callback {
    
    private final VfsDir dir;
    private final String query;
    private final RealBrowserViewModel model;
    private final Handler handler;
    private BrowserController control;
    
    private SearchingLoaderCallback(VfsDir dir, String query) {
      this.dir = dir;
      this.query = query;
      this.model = new RealBrowserViewModel(MainApplication.getInstance());
      this.handler = new Handler();
    }
    
    static LoaderManager.LoaderCallbacks<Void> create(BrowserController ctrl, VfsDir dir, String query) {
      final SearchingLoaderCallback cb = new SearchingLoaderCallback(dir, query);
      cb.control = ctrl;
      ctrl.listing.setModel(cb.model);
      ctrl.searchingStarted();
      return cb;
    }
    
    static LoaderManager.LoaderCallbacks<Void> create(BrowserController ctrl, SearchingLoader loader) {
      final SearchingLoaderCallback cb = (SearchingLoaderCallback) loader.getCallback();
      synchronized (cb) {
        cb.control = ctrl;
        ctrl.listing.setModel(cb.model);
      }
      if (loader.isStarted()) {
        ctrl.searchingStarted();
      }
      ctrl.position.setDir(cb.dir);
      return cb;
    }
    
    static void detachLoader(SearchingLoader loader) {
      final SearchingLoaderCallback cb = (SearchingLoaderCallback) loader.getCallback();
      synchronized (cb) {
        cb.control = null;
        cb.handler.removeCallbacksAndMessages(null);
      }
    }
    
    @Override
    public Loader<Void> onCreateLoader(int id, Bundle params) {
      return new SearchingLoader(MainApplication.getInstance(), dir, query, this);
    }

    @Override
    public void onLoadFinished(Loader<Void> loader, Void result) {
      loadingFinished();
    }

    @Override
    public void onLoaderReset(Loader<Void> loader) {
      loadingFinished();
    }
    
    @Override
    public synchronized void onFileFound(final VfsFile file) {
      if (control != null) {
        final BaseAdapter adapter = (BaseAdapter) control.listing.getAdapter();
        handler.post(new Runnable() {
          @Override
          public void run() {
            model.add(file);
            adapter.notifyDataSetChanged();
          }
        });
      } else {
        model.add(file);
      }
    }
    
    private synchronized void loadingFinished() {
      if (control != null) {
        control.loadingFinished();
      }
    }
  }
  
  private void listingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_loading);
  }
  
  private void searchingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_searching);
  }

  private void loadingFinished() {
    hideProgress();
    listing.setEmptyViewText(R.string.browser_empty);
  }
  
  private void loadingFailed(Exception e) {
    hideProgress();
    listing.showError(e);
  }

  private void showProgress() {
    progress.setVisibility(View.VISIBLE);
    progress.setIndeterminate(true);
  }
  
  private void hideProgress() {
    progress.setVisibility(View.INVISIBLE);
  }
}
