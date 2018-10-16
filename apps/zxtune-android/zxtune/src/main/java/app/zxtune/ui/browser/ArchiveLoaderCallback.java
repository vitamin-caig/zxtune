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
import android.support.annotation.Nullable;

import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;

class ArchiveLoaderCallback implements LoaderManager.LoaderCallbacks<Object>, ArchiveLoader.Callback {
  
  private final VfsFile file;
  private final Runnable playCmd;
  private BrowserController control;
  
  private ArchiveLoaderCallback(VfsFile file, Runnable playCmd, BrowserController control) {
    this.file = file;
    this.playCmd = playCmd;
    this.control = control;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, VfsFile file, Runnable playCmd) {
    final ArchiveLoaderCallback cb = new ArchiveLoaderCallback(file, playCmd, ctrl);
    ctrl.showProgress();
    return cb;
  }
  
  static LoaderManager.LoaderCallbacks<Object> create(BrowserController ctrl, ArchiveLoader loader) {
    final ArchiveLoaderCallback cb = (ArchiveLoaderCallback) loader.getCallback();
    cb.setControl(ctrl);
    if (loader.isStarted()) {
      ctrl.showProgress();
    }
    return cb;
  }
  
  static void detachLoader(ArchiveLoader loader) {
    final ArchiveLoaderCallback cb = (ArchiveLoaderCallback) loader.getCallback();
    cb.setControl(null);
  }

  @Override
  public Loader<Object> onCreateLoader(int id, Bundle params) {
    return new ArchiveLoader(MainApplication.getInstance(), file, this);
  }

  private synchronized void setControl(@Nullable BrowserController control) {
    this.control = control;
  }

  @Override
  public synchronized void onLoadFinished(Loader<Object> loader, Object result) {
    if (control != null) {
      if (result instanceof VfsDir) {
        control.browseDir((VfsDir) result);
      } else {
        control.archiveLoadingFinished(playCmd);
      }
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
