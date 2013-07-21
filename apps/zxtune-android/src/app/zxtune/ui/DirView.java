/*
 * @file
 * @brief DirView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.app.Activity;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.support.v4.widget.CursorAdapter;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsCursor;

public class DirView extends ListView
    implements
      AdapterView.OnItemClickListener,
      AdapterView.OnItemLongClickListener {

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
    final Uri uri = Uri.parse(cursor.getString(VfsCursor.Columns.URI));
    if (VfsCursor.Types.FILE == cursor.getInt(VfsCursor.Columns.TYPE)) {
      listener.onFileClick(uri);
    } else {
      listener.onDirClick(uri);
    }
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    final Cursor cursor = (Cursor) parent.getAdapter().getItem(position);
    final Uri uri = Uri.parse(cursor.getString(VfsCursor.Columns.URI));
    if (VfsCursor.Types.FILE == cursor.getInt(VfsCursor.Columns.TYPE)) {
      return listener.onFileLongClick(uri);
    } else {
      return listener.onDirLongClick(uri);
    }
  }

  public void setOnEntryClickListener(OnEntryClickListener listener) {
    this.listener = null != listener ? listener : new StubOnEntryClickListener();
  }

  public void setUri(Uri path) {
    setDir((Vfs.Dir) Vfs.getRoot().resolve(path));
  }

  public void setDir(Vfs.Dir dir) {
    final Cursor cursor = new VfsCursor(dir);
    ((Activity) getContext()).startManagingCursor(cursor);
    final CursorAdapter adapter = new DirViewCursorAdapter(getContext(), cursor, false);
    setAdapter(adapter);
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
      holder.name.setText(cursor.getString(VfsCursor.Columns.NAME));
      switch (cursor.getInt(VfsCursor.Columns.TYPE)) {
        case VfsCursor.Types.DIR:
          holder.name.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_browser_folder, 0, 0, 0);
          holder.size.setVisibility(GONE);
          break;
        case VfsCursor.Types.FILE:
          holder.name.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
          holder.size.setVisibility(VISIBLE);
          holder.size.setText(Long.toString(cursor.getLong(VfsCursor.Columns.SIZE)));
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
