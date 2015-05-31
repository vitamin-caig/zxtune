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
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.util.Log;
import android.view.View;
import android.widget.ProgressBar;
import app.zxtune.Preferences;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;

public class BrowserController {
  
  private final static String TAG = BrowserController.class.getName();
  private final static int LOADER_ID = BrowserController.class.hashCode();
  
  private final LoaderManager loaderManager;
  private final BrowserState state;
  BreadCrumbsView position;
  ProgressBar progress;
  BrowserView listing;

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
  
  void listingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_loading);
  }
  
  void searchingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_searching);
  }

  void loadingFinished() {
    hideProgress();
    listing.setEmptyViewText(R.string.browser_empty);
  }
  
  void loadingFailed(Exception e) {
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
