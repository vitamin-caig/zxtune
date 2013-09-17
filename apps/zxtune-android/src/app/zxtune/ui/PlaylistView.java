/*
 * @file
 * @brief PlaylistView class
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
import app.zxtune.playlist.Item;
import app.zxtune.playlist.Query;

public class PlaylistView extends ListView implements LoaderManager.LoaderCallbacks<Cursor> {

  public interface PlayitemStateSource {

    public boolean isPlaying(Uri playlistUri);
  }

  private static class StubPlayitemStateSource implements PlayitemStateSource {

    @Override
    public boolean isPlaying(Uri playlistUri) {
      return false;
    }
  }

  private static final int LOADER_ID = PlaylistView.class.hashCode();
  
  private PlayitemStateSource state;

  public PlaylistView(Context context) {
    super(context);
    setupView();
  }

  public PlaylistView(Context context, AttributeSet attr) {
    super(context, attr);
    setupView();
  }

  public PlaylistView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    setupView();
  }

  private void setupView() {
    super.setLongClickable(true);
    setAdapter(new PlaylistCursorAdapter(getContext(), null, 0));
  }
  
  public final void setPlayitemStateSource(PlayitemStateSource source) {
    this.state = null != source ? source : new StubPlayitemStateSource();
  }
  
  public final void load(LoaderManager manager) {
    manager.initLoader(LOADER_ID, null, this);
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
        if (checked.get(i)) {
          ++count;
        }
      }
      return count;
    }
  }
  
  @Override
  public Loader<Cursor> onCreateLoader(int id, Bundle params) {
    assert id == LOADER_ID;
    getCursorAdapter().changeCursor(null);
    return new CursorLoader(getContext(), Query.unparse(null), null, null, null, null);
  }

  @Override
  public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
    getCursorAdapter().changeCursor(cursor);
    final Object tag = getTag();
    if (tag != null) {
      if (tag instanceof Integer) {
        setSelection((Integer) tag);
      } else {
        onRestoreInstanceState((Parcelable) tag);
      }
      setTag(null);
    }
  }

  @Override
  public void onLoaderReset(Loader<Cursor> loader) {
    getCursorAdapter().changeCursor(null);
  }
  
  private CursorAdapter getCursorAdapter() {
    return (CursorAdapter)getAdapter();
  }

  private class PlaylistCursorAdapter extends CursorAdapter {

    private final LayoutInflater inflater;

    public PlaylistCursorAdapter(Context context, Cursor cursor, boolean autoRequery) {
      super(context, cursor, autoRequery);
      this.inflater = LayoutInflater.from(context);
    }

    public PlaylistCursorAdapter(Context context, Cursor cursor, int flags) {
      super(context, cursor, flags);
      this.inflater = LayoutInflater.from(context);
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
      final ViewHolder holder = (ViewHolder) view.getTag();
      final Item item = new Item(cursor);
      final Uri uri = item.getUri();
      if (0 == item.getTitle().length()) {
        holder.title.setText(item.getLocation().getLastPathSegment());
      } else {
        holder.title.setText(item.getTitle());
      }
      holder.author.setText(item.getAuthor());
      holder.duration.setText(item.getDuration().toString());
      final int icon = state.isPlaying(uri) ? R.drawable.ic_stat_notify_play : 0;
      holder.duration.setCompoundDrawablesWithIntrinsicBounds(icon, 0, 0, 0);
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      final View view = inflater.inflate(R.layout.playlist_item, parent, false);
      view.setTag(new ViewHolder(view));
      return view;
    }
  }

  private static class ViewHolder {

    final TextView title;
    final TextView author;
    final TextView duration;

    public ViewHolder(View view) {
      this.title = (TextView) view.findViewById(R.id.playlist_item_title);
      this.author = (TextView) view.findViewById(R.id.playlist_item_author);
      this.duration = (TextView) view.findViewById(R.id.playlist_item_duration);
    }
  }
}
