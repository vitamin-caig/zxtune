/**
 *
 * @file
 *
 * @brief File browser view component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import java.util.Comparator;

import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ProgressBar;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

public class BrowserView extends ListViewCompat {
  
  private static final String TAG = BrowserView.class.getName();

  private static final int LOADER_ID = BrowserView.class.hashCode();

  private ProgressBar loadingView;
  private TextView emptyView;

  public BrowserView(Context context) {
    super(context);
    initView();
  }

  public BrowserView(Context context, AttributeSet attr) {
    super(context, attr);
    initView();
  }

  public BrowserView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    initView();
  }
  
  private void initView() {
    setAdapter(new BrowserViewAdapter());
  }

  @Override
  public void setEmptyView(View stub) {
    super.setEmptyView(stub);
    loadingView = (ProgressBar) stub.findViewById(R.id.browser_loading);
    emptyView = (TextView) stub.findViewById(R.id.browser_loaded);
  }
  
  //Required to call forceLoad due to bug in support library.
  //Some methods on callback does not called... 
  final void loadNew(LoaderManager manager, VfsDir dir, int pos) {
    manager.destroyLoader(LOADER_ID);
    final ModelLoaderCallback cb = new ModelLoaderCallback(dir, pos);
    manager.initLoader(LOADER_ID, null, cb).forceLoad();
  }

  //load existing
  final boolean loadCurrent(LoaderManager manager) {
    final Loader<BrowserViewModel> loader = manager.getLoader(LOADER_ID);
    if (loader == null) {
      Log.d(TAG, "Expired loader");
      return false;
    }
    if (loader.isStarted()) {
      showProgress();
    }
    final ModelLoaderCallback cb = new ModelLoaderCallback();
    manager.initLoader(LOADER_ID, null, cb);
    return true;
  }
  
  final void showError(Exception e) {
    setModel(null);
    final Throwable cause = e.getCause();
    final String msg = cause != null ? cause.getMessage() : e.getMessage();
    emptyView.setText(msg);
  }
  
  private void setModel(BrowserViewModel model) {
    ((BrowserViewAdapter) getAdapter()).setModel(model);
  }

  //TODO: use ViewFlipper?
  private void showProgress() {
    loadingView.setIndeterminate(true);
    loadingView.setVisibility(VISIBLE);
    emptyView.setVisibility(INVISIBLE);
  }

  private void hideProgress() {
    loadingView.setVisibility(INVISIBLE);
    emptyView.setVisibility(VISIBLE);
  }
  
  private static class BrowserViewAdapter extends BaseAdapter {
    
    private BrowserViewModel model;
    
    BrowserViewAdapter() {
      this.model = getSafeModel(null);
    }

    @Override
    public int getCount() {
      return model.getCount();
    }

    @Override
    public Object getItem(int position) {
      return model.getItem(position);
    }

    @Override
    public long getItemId(int position) {
      return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      return model.getView(position, convertView, parent);
    }

    @Override
    public boolean isEmpty() {
      return model.isEmpty();
    }
    
    final void setModel(BrowserViewModel model) {
      if (this.model == model) {
        return;
      }
      this.model = getSafeModel(model);
      if (model != null) {
        notifyDataSetChanged();
      } else {
        notifyDataSetInvalidated();
      }
    }
    
    private BrowserViewModel getSafeModel(BrowserViewModel model) {
      if (model != null) {
        return model;
      } else {
        return new EmptyBrowserViewModel();
      }
    }
  }
  
  private class ModelLoaderCallback implements LoaderManager.LoaderCallbacks<BrowserViewModel> {
    
    private final VfsDir dir;
    private final Integer pos;
    
    ModelLoaderCallback() {
      this.dir = null;
      this.pos = null;
    }
    
    ModelLoaderCallback(VfsDir dir, Integer pos) {
      this.dir = dir;
      this.pos = pos;
    }
    
    @Override
    public Loader<BrowserViewModel> onCreateLoader(int id, Bundle params) {
      assert id == LOADER_ID;
      assert dir != null;
      showProgress();
      setModel(null);
      return new ModelLoader(getContext(), dir, BrowserView.this);
    }

    @Override
    public void onLoadFinished(Loader<BrowserViewModel> loader, BrowserViewModel model) {
      hideProgress();
      setModel(model);
      if (pos != null) {
        setSelection(pos);
      }
    }

    @Override
    public void onLoaderReset(Loader<BrowserViewModel> loader) {
      hideProgress();
    }
  }
  
  //Use from android.os when api16 will be minimal
  private static class CancellationSignal {
    
    private volatile boolean canceled;
    
    final void cancel() {
      canceled = true;
    }
    
    final void throwIfCanceled() {
      if (canceled) {
        throw new OperationCanceledException();
      }
    }
  }
  
  private static class OperationCanceledException extends RuntimeException {
    private static final long serialVersionUID = 1L;
  }
  
  //Typical AsyncTaskLoader workflow from
  //http://developer.android.com/intl/ru/reference/android/content/AsyncTaskLoader.html
  //Must be static!!!
  private static class ModelLoader extends AsyncTaskLoader<BrowserViewModel> {
    
    private final VfsDir dir;
    private final BrowserView view;
    private CancellationSignal signal;

    ModelLoader(Context context, VfsDir dir, BrowserView view) {
      super(context);
      this.dir = dir;
      this.view = view;
      this.signal = new CancellationSignal();
      view.emptyView.setText(R.string.browser_empty);
    }
    
    @Override
    protected void onReset() {
      super.onReset();
      signal.cancel();
      Log.d(TAG, "Reset loader");
    }
    
    @SuppressWarnings("unchecked")
    @Override
    public BrowserViewModel loadInBackground() {
      final RealBrowserViewModel model = new RealBrowserViewModel(getContext());
      try {
        dir.enumerate(new VfsDir.Visitor() {
          
          int counter;
          
          @Override
          public void onItemsCount(int count) {
            view.loadingView.setIndeterminate(false);
            view.loadingView.setMax(count);
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
            if (++counter % 10 == 0) {
              view.post(new Runnable() {
                @Override
                public void run() {
                  view.loadingView.setProgress(counter);
                }
              });
            }
          }
        });
        if (dir instanceof Comparator<?>) {
          model.sort((Comparator<VfsObject>)dir);
        } else {
          model.sort();
        }
        return model;
      } catch (OperationCanceledException e) {
        return model;
      } catch (final Exception e) {
        view.post(new Runnable() {
          @Override
          public void run() {
            view.showError(e);
          }
        });
      }
      return null;
    }
  }
}
