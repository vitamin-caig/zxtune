/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.widget.BaseAdapter;

import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;

class SearchingLoaderCallback implements LoaderManager.LoaderCallbacks<Void>, SearchingLoader.Callback {
  
  private final VfsDir dir;
  private final String query;
  private final RealBrowserViewModel model;
  private final Handler handler;
  private BrowserController control;
  
  private SearchingLoaderCallback(VfsDir dir, String query, BrowserController control) {
    this.dir = dir;
    this.query = query;
    this.model = new RealBrowserViewModel(MainApplication.getInstance());
    this.handler = new Handler();
    this.control = control;
  }
  
  static LoaderManager.LoaderCallbacks<Void> create(BrowserController ctrl, VfsDir dir, String query) {
    final SearchingLoaderCallback cb = new SearchingLoaderCallback(dir, query, ctrl);
    ctrl.listing.setModel(cb.model);
    ctrl.searchingStarted();
    return cb;
  }
  
  static LoaderManager.LoaderCallbacks<Void> create(BrowserController ctrl, SearchingLoader loader) {
    final SearchingLoaderCallback cb = (SearchingLoaderCallback) loader.getCallback();
    cb.setControl(ctrl);
    if (loader.isStarted()) {
      ctrl.searchingStarted();
    }
    ctrl.position.setDir(cb.dir);
    return cb;
  }
  
  static void detachLoader(SearchingLoader loader) {
    final SearchingLoaderCallback cb = (SearchingLoaderCallback) loader.getCallback();
    cb.setControl(null);
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

  private synchronized void setControl(@Nullable BrowserController control) {
    if (control != null) {
      this.control = control;
      control.listing.setModel(this.model);
    } else {
      this.control = null;
      this.handler.removeCallbacksAndMessages(null);
    }
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
