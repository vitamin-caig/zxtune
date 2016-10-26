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
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;

class ArchiveLoaderCallback implements LoaderManager.LoaderCallbacks<Object>, ArchiveLoader.Callback {
  
  private final VfsFile file;
  private final Runnable playCmd;
  private BrowserController control;
  
  private ArchiveLoaderCallback(VfsFile file, Runnable playCmd) {
    this.file = file;
    this.playCmd = playCmd;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, VfsFile file, Runnable playCmd) {
    final ArchiveLoaderCallback cb = new ArchiveLoaderCallback(file, playCmd);
    cb.control = ctrl;
    ctrl.showProgress();
    return cb;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, ArchiveLoader loader) {
    final ArchiveLoaderCallback cb = (ArchiveLoaderCallback) loader.getCallback();
    synchronized (cb) {
      cb.control = ctrl;
    }
    if (loader.isStarted()) {
      ctrl.showProgress();
    }
    return cb;
  }
  
  static void detachLoader(ArchiveLoader loader) {
    final ArchiveLoaderCallback cb = (ArchiveLoaderCallback) loader.getCallback();
    synchronized (cb) {
      cb.control = null;
    }
  }
  
  @Override
  public Loader<Object> onCreateLoader(int id, Bundle params) {
    return new ArchiveLoader(MainApplication.getInstance(), file, this);
  }

  @Override
  public void onLoadFinished(Loader<Object> loader, Object result) {
    if (control == null) {
      return;
    } else if (result instanceof VfsDir) {
      control.browseArchiveRoot((VfsDir) result);
    } else {
      control.archiveLoadingFinished(playCmd);
    }
  }

  @Override
  public void onLoaderReset(Loader<Object> loader) {
    loadingFinished();
  }
  
  private synchronized void loadingFinished() {
    if (control != null) {
      control.hideProgress();
    }
  }
}
