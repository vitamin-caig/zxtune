/**
 * 
 * @file
 *
 * @brief Async loader for search results 
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import android.content.Context;
import android.net.Uri;

import java.io.IOException;

import androidx.loader.content.AsyncTaskLoader;
import androidx.core.os.CancellationSignal;
import androidx.core.os.OperationCanceledException;
import app.zxtune.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;

class SearchingLoader extends AsyncTaskLoader<Void> {
  
  public interface Callback {
    void onFileFound(VfsFile file);
  }

  private static final String TAG = SearchingLoader.class.getName();
  private final VfsDir dir;
  private final String query;
  private final Callback cb;
  private final CancellationSignal signal;

  SearchingLoader(Context context, VfsDir dir, String query, Callback cb) {
    super(context);
    this.dir = dir;
    this.query = query.toLowerCase();
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
    Log.d(TAG, "Reset search loader");
  }

  @Override
  public Void loadInBackground() {
    try {
      if (!optimizedSearch()) {
        for (final VfsIterator iter = new VfsIterator(new Uri[] {dir.getUri()}); iter.isValid(); iter.next()) {
          signal.throwIfCanceled();
          final VfsFile file = iter.getFile();
          if (matchQuery(file.getName()) || matchQuery(file.getDescription())) {
            cb.onFileFound(file);
          }
        }
      }
    } catch (OperationCanceledException e) {
      Log.w(TAG, e, "Search is canceled");
    }
    return null;
  }

  private boolean matchQuery(String txt) {
    return txt.toLowerCase().contains(query); 
  }
  
  private boolean optimizedSearch() {
    final VfsExtensions.SearchEngine engine = (VfsExtensions.SearchEngine) dir.getExtension(VfsExtensions.SEARCH_ENGINE);
    if (engine == null) {
      return false;
    }
    try {
        engine.find(query, new VfsExtensions.SearchEngine.Visitor() {
          @Override
          public void onFile(VfsFile file) {
            signal.throwIfCanceled();
            cb.onFileFound(file);
          }
        });
    } catch(IOException e) {
      return false;
    }
    return true;
  }
}
