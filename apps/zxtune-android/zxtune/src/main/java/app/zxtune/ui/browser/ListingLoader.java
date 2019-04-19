/**
 * 
 * @file
 *
 * @brief Async loader for plain directory content
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import android.content.Context;
import android.support.annotation.Nullable;

import java.util.Comparator;

import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.os.CancellationSignal;
import android.support.v4.os.OperationCanceledException;
import app.zxtune.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

//Typical AsyncTaskLoader work flow from
//http://developer.android.com/intl/ru/reference/android/content/AsyncTaskLoader.html

// Returns BrowserViewModel or Exception
class ListingLoader extends AsyncTaskLoader<Object> {

  public interface Callback {
    void onProgressInit(int total);
    void onProgressUpdate(int current);
  }
  
  private static final String TAG = ListingLoader.class.getName();
  @Nullable private final VfsDir dir;
  private final Callback cb;
  private final CancellationSignal signal;

  ListingLoader(Context context, @Nullable VfsDir dir, Callback cb) {
    super(context);
    this.dir = dir;
    this.cb = cb;
    this.signal = new CancellationSignal();
  }
  
  final Callback getCallback() {
    return cb;
  }

  @Override
  protected void onReset() {
    super.onReset();
    signal.cancel();
    Log.d(TAG, "Reset listing loader");
  }

  @SuppressWarnings("unchecked")
  @Override
  public Object loadInBackground() {
    if (dir == null) {
      return new EmptyBrowserViewModel();
    }
    final RealBrowserViewModel model = new RealBrowserViewModel(getContext());
    try {
      dir.enumerate(new VfsDir.Visitor() {

        int counter;

        @Override
        public void onItemsCount(int count) {
          cb.onProgressInit(count);
        }

        @Override
        public void onFile(VfsFile file) {
          model.add(file);
          updateProgress();
          signal.throwIfCanceled();
        }

        @Override
        public void onDir(VfsDir dir) {
          model.add(dir);
          updateProgress();
          signal.throwIfCanceled();
        }

        private void updateProgress() {
          cb.onProgressUpdate(++counter);
        }
      });
      final Object extension = dir.getExtension(VfsExtensions.COMPARATOR);
      if (extension instanceof Comparator<?>) {
        model.sort((Comparator<VfsObject>) extension);
      } else {
        model.sort();
      }
      return model;
    } catch (OperationCanceledException e) {
      return model;
    } catch (Exception e) {
      return e;
    }
  }
}
