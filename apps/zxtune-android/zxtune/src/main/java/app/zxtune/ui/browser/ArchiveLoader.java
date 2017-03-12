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

import android.content.Context;
import android.support.v4.content.AsyncTaskLoader;

import app.zxtune.Log;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsFile;

public class ArchiveLoader extends AsyncTaskLoader<Object> {
  
  public interface Callback {
    //TODO: track progress
  }

  private static final String TAG = ArchiveLoader.class.getName();
  private final VfsFile file;
  private final Callback cb;

  ArchiveLoader(Context context, VfsFile file, Callback cb) {
    super(context);
    this.file = file;
    this.cb = cb;
  }
  
  final Callback getCallback() {
    return cb;
  }
  
  @Override
  public Object loadInBackground() {
    Log.d(TAG, "Try to browse file as archive");
    return VfsArchive.browse(file);
  }
}
