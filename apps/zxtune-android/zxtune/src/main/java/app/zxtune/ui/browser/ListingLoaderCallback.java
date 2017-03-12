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
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.widget.ProgressBar;

import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir;

class ListingLoaderCallback implements LoaderManager.LoaderCallbacks<Object>, ListingLoader.Callback {
  
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
  public synchronized void onProgressInit(final int total) {
    if (control != null) {
      final ProgressBar progress = control.progress;
      handler.post(new Runnable() {
        @Override
        public void run() {
          progress.setProgress(0);
          progress.setMax(total);
          progress.setIndeterminate(false);
        }
      });
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
