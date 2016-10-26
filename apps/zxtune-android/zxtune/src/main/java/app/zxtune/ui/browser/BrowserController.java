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

import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.view.View;
import android.widget.ProgressBar;

import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.Preferences;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

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
    } else if (loader instanceof ArchiveLoader) {
      ArchiveLoaderCallback.detachLoader((ArchiveLoader) loader);
    }
  }
  
  public final void loadState() {
    if (!reloadCurrentState()) {
      loadCurrentDir();
    }
  }
  
  public final void search(String query) {
    try {
      final VfsDir currentDir = getCurrentDir();
      loaderManager.destroyLoader(LOADER_ID);
      final LoaderManager.LoaderCallbacks<?> cb = SearchingLoaderCallback.create(this, currentDir, query);
      loaderManager.initLoader(LOADER_ID, null, cb).forceLoad();
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to search");
      listing.showError(e);
    }
  }
  
  public final boolean isInSearch() {
    final Loader<?> loader = loaderManager.getLoader(LOADER_ID); 
    return loader instanceof SearchingLoader;
  }

  public final void browseDir(VfsDir dir) {
    setCurrentDir(dir);
    Analytics.sendBrowseEvent(dir);
  }

  private void setCurrentDir(VfsDir dir) {
    storeCurrentViewPosition();
    state.setCurrentPath(dir.getUri());
    setDirectory(dir);
  }
  
  public final void moveToParent() {
    try {
      final VfsDir root = Vfs.getRoot();
      final VfsDir curDir = getCurrentDir();
      if (curDir != root) {
        final VfsDir parent = curDir != null ? (VfsDir) curDir.getParent() : null;
        setCurrentDir(parent != null ? parent : root);
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to move up");
      listing.showError(e);
    }
  }
  
  public final void storeCurrentViewPosition() {
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }

  public final void browseArchive(VfsFile file, Runnable playCmd) {
    final VfsObject resolved = VfsArchive.browseCached(file);
    if (resolved == null) {
      loadArchive(file, playCmd);
    } else if (resolved instanceof VfsDir) {
      browseDir((VfsDir) resolved);
    } else if (resolved instanceof VfsFile) {
      playCmd.run();
    }
  }

  private boolean reloadCurrentState() {
    final Loader<?> loader = loaderManager.getLoader(LOADER_ID); 
    if (loader == null) {
      Log.d(TAG, "Expired loader");
      return false;
    }
    if (loader instanceof SearchingLoader) {
      loaderManager.initLoader(LOADER_ID, null, SearchingLoaderCallback.create(this, (SearchingLoader) loader));
    } else if (loader instanceof ListingLoader) {
      loaderManager.initLoader(LOADER_ID, null, ListingLoaderCallback.create(this, (ListingLoader) loader));
    } else if (loader instanceof ArchiveLoader) {
      loaderManager.initLoader(LOADER_ID, null, ArchiveLoaderCallback.create(this, (ArchiveLoader) loader));
    }
    return true;
  }
  
  public final void loadCurrentDir() {
    try {
      setDirectory(getCurrentDir());
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to load current dir");
      listing.showError(e);
    }
  }
  
  private VfsDir getCurrentDir() throws IOException {
    final VfsObject obj = VfsArchive.resolve(state.getCurrentPath());
    if (obj instanceof VfsDir) {
      return (VfsDir) obj;
    } else {
      return null;
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
  
  private void loadArchive(VfsFile file, Runnable playCmd) {
    loaderManager.destroyLoader(LOADER_ID);
    final LoaderManager.LoaderCallbacks<?> cb = ArchiveLoaderCallback.create(this, file, playCmd);
    loaderManager.initLoader(LOADER_ID, null, cb).forceLoad();
  }
  
  final void listingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_loading);
  }
  
  final void searchingStarted() {
    showProgress();
    listing.setEmptyViewText(R.string.browser_searching);
  }

  final void loadingFinished() {
    hideProgress();
    listing.setEmptyViewText(R.string.browser_empty);
  }
  
  final void archiveLoadingFinished(Runnable playCmd) {
    loaderManager.destroyLoader(LOADER_ID);
    hideProgress();
    playCmd.run();
  }
  
  final void loadingFailed(Exception e) {
    hideProgress();
    Log.w(TAG, e, "Failed to load dir");
    listing.showError(e);
  }

  final void showProgress() {
    progress.setVisibility(View.VISIBLE);
    progress.setIndeterminate(true);
  }
  
  final void hideProgress() {
    progress.setVisibility(View.INVISIBLE);
  }
}
