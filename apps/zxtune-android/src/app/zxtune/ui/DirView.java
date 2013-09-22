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
import android.os.Build;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.util.AttributeSet;
import android.util.SparseBooleanArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsQuery;

public class DirView extends ListView implements LoaderManager.LoaderCallbacks<Cursor> {

  private static final int LOADER_ID = DirView.class.hashCode();

  private View loadingView;
  private View emptyView;

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
    super.setLongClickable(true);
    setAdapter(new DirViewCursorAdapter(getContext(), null, 0));
  }

  @Override
  public void setEmptyView(View stub) {
    super.setEmptyView(stub);
    loadingView = stub.findViewById(R.id.browser_loading);
    emptyView = stub.findViewById(R.id.browser_loaded);
  }

  public final void load(LoaderManager manager) {
    if (getTag() != null) {
      manager.restartLoader(LOADER_ID, null, this);
    } else {
      assert manager.getLoader(LOADER_ID) != null;
      manager.initLoader(LOADER_ID, null, this);
    }
  }

  public final void selectAll() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, true);
    }
  }

  public final void selectNone() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, false);
    }
  }

  public final void invertSelection() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, !isItemChecked(i));
    }
  }

  @Override
  public int getCheckedItemCount() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      //since API11
      return super.getCheckedItemCount();
    } else {
      final SparseBooleanArray checked = getCheckedItemPositions();
      int count = 0;
      for (int i = 0, lim = checked.size(); i != lim; ++i) {
        if (checked.valueAt(i)) {
          ++count;
        }
      }
      return count;
    }
  }
  
  @Override
  public void onRestoreInstanceState(Parcelable state) {
    super.onRestoreInstanceState(state);
  }

  @Override
  public Loader<Cursor> onCreateLoader(int id, Bundle params) {
    assert id == LOADER_ID;
    showProgress();
    getCursorAdapter().changeCursor(null);
    final Uri path = ((BrowserState) getTag()).getCurrentPath();
    final Uri query = VfsQuery.unparse(path);
    return new CursorLoader(getContext(), query, null, null, null, null);
  }

  @Override
  public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
    hideProgress();
    getCursorAdapter().changeCursor(cursor);
    final BrowserState state = (BrowserState) getTag();
    if (state != null) {
      setSelection(state.getCurrentViewPosition());
      setTag(null);
    }
  }

  @Override
  public void onLoaderReset(Loader<Cursor> loader) {
    hideProgress();
    getCursorAdapter().changeCursor(null);
  }

  private CursorAdapter getCursorAdapter() {
    return (CursorAdapter) getAdapter();
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
          holder.name
              .setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_browser_folder, 0, 0, 0);
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
