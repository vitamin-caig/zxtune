/*
 * @file
 * @brief DirView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsQuery;

public class DirView extends ListView
    implements
      AdapterView.OnItemClickListener,
      AdapterView.OnItemLongClickListener,
      LoaderManager.LoaderCallbacks<Cursor> {

  public interface OnEntryClickListener {

    public void onDirClick(Uri uri);

    public void onFileClick(Uri uri);

    public boolean onDirLongClick(Uri uri);

    public boolean onFileLongClick(Uri uri);
  }

  public static class StubOnEntryClickListener implements OnEntryClickListener {

    @Override
    public void onDirClick(Uri uri) {}

    @Override
    public void onFileClick(Uri uri) {}

    @Override
    public boolean onDirLongClick(Uri uri) {
      return false;
    }

    @Override
    public boolean onFileLongClick(Uri uri) {
      return false;
    }
  }
  
  private static final int LOADER_ID = DirView.class.hashCode();
  private static final String LOADER_PARAM_PATH = "path";

  private CursorAdapter adapter;
  private OnEntryClickListener listener;

  public DirView(Context context) {
    super(context);
    setupView();
  }

  public DirView(Context context, AttributeSet attr) {
    super(context, attr);
    setupView();
  }

  public DirView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    setupView();
  }

  private void setupView() {
    listener = new StubOnEntryClickListener();
    super.setLongClickable(true);
    super.setOnItemClickListener(this);
    super.setOnItemLongClickListener(this);
    adapter = new DirViewCursorAdapter(getContext(), null, 0);
    setAdapter(adapter);
  }

  @Override
  public void setOnItemClickListener(OnItemClickListener l) {
    throw new UnsupportedOperationException();
  }

  @Override
  public void setOnItemLongClickListener(OnItemLongClickListener l) {
    throw new UnsupportedOperationException();
  }

  @Override
  public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
    final Cursor cursor = (Cursor) parent.getAdapter().getItem(position);
    final Uri uri = Uri.parse(cursor.getString(VfsQuery.Columns.URI));
    if (VfsQuery.Types.FILE == cursor.getInt(VfsQuery.Columns.TYPE)) {
      listener.onFileClick(uri);
    } else {
      listener.onDirClick(uri);
    }
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    final Cursor cursor = (Cursor) parent.getAdapter().getItem(position);
    final Uri uri = Uri.parse(cursor.getString(VfsQuery.Columns.URI));
    if (VfsQuery.Types.FILE == cursor.getInt(VfsQuery.Columns.TYPE)) {
      return listener.onFileLongClick(uri);
    } else {
      return listener.onDirLongClick(uri);
    }
  }

  public final void setOnEntryClickListener(OnEntryClickListener listener) {
    this.listener = null != listener ? listener : new StubOnEntryClickListener();
  }
  
  public final void setUri(Uri path) {
    //TODO: temporal method for popup window
    final Cursor cursor = getContext().getContentResolver().query(VfsQuery.unparse(path), null, null, null, null);
    adapter.changeCursor(cursor);
  }

  public final void setUri(LoaderManager manager, Uri path) {
    final Bundle params = new Bundle();
    params.putParcelable(LOADER_PARAM_PATH, path);
    manager.restartLoader(LOADER_ID, params, this);
  }

  @Override
  public Loader<Cursor> onCreateLoader(int id, Bundle params) {
    if (id == LOADER_ID) {
      final Uri path = (Uri)params.getParcelable(LOADER_PARAM_PATH);
      return new CursorLoader(getContext(), VfsQuery.unparse(path), null, null, null, null);
    } else {
      return null;
    }
  }

  @Override
  public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
    adapter.changeCursor(cursor);
  }

  @Override
  public void onLoaderReset(Loader<Cursor> loader) {
    adapter.changeCursor(null);
  }

  private static class DirViewCursorAdapter extends CursorAdapter {

    private final LayoutInflater inflater;

    public DirViewCursorAdapter(Context context, Cursor cursor, boolean autoRequery) {
      super(context, cursor, autoRequery);
      inflater = LayoutInflater.from(context);
    }

    public DirViewCursorAdapter(Context context, Cursor cursor, int flags) {
      super(context, cursor, flags);
      inflater = LayoutInflater.from(context);
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
      final ViewHolder holder = (ViewHolder) view.getTag();
      holder.name.setText(cursor.getString(VfsQuery.Columns.NAME));
      switch (cursor.getInt(VfsQuery.Columns.TYPE)) {
        case VfsQuery.Types.DIR:
          holder.name.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_browser_folder, 0, 0, 0);
          holder.size.setVisibility(GONE);
          break;
        case VfsQuery.Types.FILE:
          holder.name.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
          holder.size.setVisibility(VISIBLE);
          holder.size.setText(Long.toString(cursor.getLong(VfsQuery.Columns.SIZE)));
          break;
      }
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      final View res = inflater.inflate(R.layout.dirview_item, parent, false);
      final ViewHolder holder = new ViewHolder();
      holder.name = (TextView) res.findViewById(R.id.dirview_item_name);
      holder.size = (TextView) res.findViewById(R.id.dirview_item_size);
      res.setTag(holder);
      return res;
    }
    
    private static class ViewHolder {
      public TextView name;
      public TextView size;
    }
  }
}
