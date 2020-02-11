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
import androidx.annotation.Nullable;
import androidx.loader.app.LoaderManager;
import androidx.loader.content.Loader;
import android.widget.ProgressBar;

import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir;
import app.zxtune.ui.utils.ListViewTools;

class ListingLoaderCallback implements LoaderManager.LoaderCallbacks<Object>, ListingLoader.Callback {
  
  @Nullable private final VfsDir dir;
  private final Handler handler;
  private BrowserController control;
  
  private ListingLoaderCallback(@Nullable VfsDir dir, BrowserController control) {
    this.dir = dir;
    this.handler = new Handler();
    this.control = control;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, @Nullable VfsDir dir) {
    final ListingLoaderCallback cb = new ListingLoaderCallback(dir, ctrl);
    ctrl.setModel(null);
    ctrl.listingStarted();
    return cb;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, ListingLoader loader) {
    final ListingLoaderCallback cb = (ListingLoaderCallback) loader.getCallback();
    cb.setControl(ctrl);
    if (loader.isStarted()) {
      ctrl.listingStarted();
    }
    return cb;
  }
  
  static void detachLoader(ListingLoader loader) {
    final ListingLoaderCallback cb = (ListingLoaderCallback) loader.getCallback();
    cb.setControl(null);
  }

  @Override
  public Loader<Object> onCreateLoader(int id, Bundle params) {
    return new ListingLoader(MainApplication.getInstance(), dir, this);
  }

  private synchronized void setControl(@Nullable BrowserController control) {
    this.control = control;
    if (control == null) {
      this.handler.removeCallbacksAndMessages(null);
    }
  }

  @Override
  public synchronized void onLoadFinished(Loader<Object> loader, Object result) {
    if (control != null) {
      if (result instanceof BrowserViewModel) {
        control.loadingFinished();
        control.setModel((BrowserViewModel) result);
        ListViewTools.useStoredViewPosition(control.listing);
      } else {
        control.loadingFailed((Exception) result);
      }
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
