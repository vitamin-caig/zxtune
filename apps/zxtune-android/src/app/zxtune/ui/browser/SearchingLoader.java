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

import java.io.IOException;

import android.content.Context;
import android.net.Uri;
import android.support.v4.content.AsyncTaskLoader;
import android.util.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;

public class SearchingLoader extends AsyncTaskLoader<Void> {
  
  public interface Callback {
    public void onFileFound(VfsFile file);
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
      for (final VfsIterator iter = new VfsIterator(new Uri[] {dir.getUri()}); iter.isValid(); iter.next()) {
        signal.throwIfCanceled();
        final VfsFile file = iter.getFile();
        if (matchQuery(file.getName()) || matchQuery(file.getDescription())) {
          cb.onFileFound(file);
        }
      }
    } catch (OperationCanceledException e) {
    }
    return null;
  }

  private boolean matchQuery(String txt) {
    return txt.toLowerCase().contains(query); 
  }
}
