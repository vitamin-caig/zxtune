/*
 * @file
 * @brief BrowserView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;

public class BrowserView extends CheckableListView implements LoaderManager.LoaderCallbacks<ListAdapter> {

  private static final int LOADER_ID = BrowserView.class.hashCode();

  private VfsRoot root;
  private View loadingView;
  private TextView emptyView;

  public BrowserView(Context context) {
    super(context);
    setupView();
  }

  public BrowserView(Context context, AttributeSet attr) {
    super(context, attr);
    setupView();
  }

  public BrowserView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    setupView();
  }

  private void setupView() {
    //super.setLongClickable(true);
  }

  @Override
  public void setEmptyView(View stub) {
    super.setEmptyView(stub);
    loadingView = stub.findViewById(R.id.browser_loading);
    emptyView = (TextView) stub.findViewById(R.id.browser_loaded);
  }
  
  final void setVfsRoot(VfsRoot root) {
    this.root = root;
  }

  final void load(LoaderManager manager) {
    if (getTag() != null) {
      manager.restartLoader(LOADER_ID, null, this).forceLoad();
    } else {
      assert manager.getLoader(LOADER_ID) != null;
      manager.initLoader(LOADER_ID, null, this).forceLoad();
    }
  }
  
  @Override
  public Loader<ListAdapter> onCreateLoader(int id, Bundle params) {
    assert id == LOADER_ID;
    showProgress();
    setAdapter(null);
    final Uri path = ((BrowserState) getTag()).getCurrentPath();
    return new BrowserLoader(getContext(), root, path, emptyView);
  }

  @Override
  public void onLoadFinished(Loader<ListAdapter> loader, ListAdapter adapter) {
    hideProgress();
    setAdapter(adapter);
    final BrowserState state = (BrowserState) getTag();
    if (state != null) {
      setSelection(state.getCurrentViewPosition());
      setTag(null);
    }
  }

  @Override
  public void onLoaderReset(Loader<ListAdapter> loader) {
    hideProgress();
  }

  //TODO: use ViewFlipper?
  private void showProgress() {
    loadingView.setVisibility(VISIBLE);
    emptyView.setVisibility(INVISIBLE);
  }

  private void hideProgress() {
    loadingView.setVisibility(INVISIBLE);
    emptyView.setVisibility(VISIBLE);
  }
  
  //Typical AsyncTaskLoader workflow from
  //http://developer.android.com/intl/ru/reference/android/content/AsyncTaskLoader.html
  private static class BrowserLoader extends AsyncTaskLoader<ListAdapter> {
    
    private final VfsRoot root;
    private final Uri path;
    private final TextView status;

    public BrowserLoader(Context context, VfsRoot root, Uri path, TextView status) {
      super(context);
      this.root = root;
      this.path = path;
      this.status = status;
      status.setText(R.string.playlist_empty);
    }
    
    @Override
    public ListAdapter loadInBackground() {
      try {
        final VfsDir dir = (VfsDir) root.resolve(path);
        final BrowserModel model = new BrowserModel(getContext());
        dir.enumerate(new VfsDir.Visitor() {
          
          @Override
          public Status onFile(VfsFile file) {
            model.add(file);
            return VfsDir.Visitor.Status.CONTINUE;
          }
          
          @Override
          public Status onDir(VfsDir dir) {
            model.add(dir);
            return VfsDir.Visitor.Status.CONTINUE;
          }
        });
        model.commit();
        return model;
      } catch (final Exception e) {
        status.post(new Runnable() {
          @Override
          public void run() {
            final Throwable cause = e.getCause();
            final String msg = cause != null ? cause.getMessage() : e.getMessage();
            status.setText(msg);
          }
        });
      }
      return null;
    }
  }
  
  //TODO: synchronize?
  private static class BrowserModel extends BaseAdapter {
    
    private final static Comparator<VfsObject> COMPARE_BY_NAME = new Comparator<VfsObject>() {

      @Override
      public int compare(VfsObject lh, VfsObject rh) {
        return String.CASE_INSENSITIVE_ORDER.compare(lh.getName(), rh.getName());
      }
    };
    
    private final LayoutInflater inflater;
    private final ArrayList<VfsDir> dirs;
    private final ArrayList<VfsFile> files;
    
    BrowserModel(Context context) {
      this.inflater = LayoutInflater.from(context);
      this.dirs = new ArrayList<VfsDir>();
      this.files = new ArrayList<VfsFile>();
    }
    
    final void add(VfsDir dir) {
      dirs.add(dir);
    }
    
    final void add(VfsFile file) {
      files.add(file);
    }
    
    final void commit() {
      Collections.sort(dirs, COMPARE_BY_NAME);
      Collections.sort(files, COMPARE_BY_NAME);
    }
    
    @Override
    public int getCount() {
      return dirs.size() + files.size();
    }

    @Override
    public Object getItem(int position) {
      final int dirsCount = dirs.size();
      if (position < dirsCount) {
        return dirs.get(position);
      } else {
        return files.get(position - dirsCount);
      }
    }

    @Override
    public long getItemId(int position) {
      return position;
    }

    //TODO: use explicit multiple view types
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      ViewHolder holder = null;
      if (convertView == null) {
        convertView = inflater.inflate(R.layout.dirview_item, parent, false);
        holder = new ViewHolder(convertView);
        convertView.setTag(holder);
      } else {
        holder = (ViewHolder) convertView.getTag();
      }

      final int dirsCount = dirs.size();
      if (position < dirsCount) {
        holder.makeView(dirs.get(position));
      } else {
        holder.makeView(files.get(position - dirsCount));
      }

      return convertView;
    }

    @Override
    public boolean isEmpty() {
      return dirs.isEmpty() && files.isEmpty();
    }
    
    private static class ViewHolder {

      private final TextView name;
      private final TextView size;
      
      ViewHolder(View view) {
        this.name = (TextView) view.findViewById(R.id.dirview_item_name);
        this.size = (TextView) view.findViewById(R.id.dirview_item_size);
      }
      
      void makeView(VfsDir dir) {
        makeBaseView(dir);
        name.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_browser_folder, 0, 0, 0);
        size.setVisibility(GONE);
      }
      
      void makeView(VfsFile file) {
        makeBaseView(file);
        name.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
        size.setVisibility(VISIBLE);
        size.setText(file.getSize());
      }
      
      private void makeBaseView(VfsObject obj) {
        name.setText(obj.getName());
      }
    }
  }
}
