/**
 * 
 * @file
 * 
 * @brief Playlist view component
 * 
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.support.v4.widget.CursorAdapter;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

import com.mobeta.android.dslv.DragSortCursorAdapter;
import com.mobeta.android.dslv.DragSortListView;

import app.zxtune.R;
import app.zxtune.playlist.Item;
import app.zxtune.playlist.PlaylistQuery;

public class PlaylistView extends DragSortListView
    implements
      LoaderManager.LoaderCallbacks<Cursor> {

  interface PlayitemStateSource {

    boolean isPlaying(Uri playlistUri);
  }

  private static class StubPlayitemStateSource implements PlayitemStateSource {

    @Override
    public boolean isPlaying(Uri playlistUri) {
      return false;
    }
  }

  private static final int LOADER_ID = PlaylistView.class.hashCode();

  private PlayitemStateSource state;

  public PlaylistView(Context context, AttributeSet attr) {
    super(context, attr);
    setupView();
  }

  private void setupView() {
    super.setLongClickable(true);
    setAdapter(new PlaylistCursorAdapter(getContext(), null, 0));
  }
  
  @Override
  public void setDropListener(DropListener listener) {
    final PlaylistCursorAdapter adapter = (PlaylistCursorAdapter) getInputAdapter();
    if (adapter != listener && adapter != null) {
      adapter.setDropListener(listener);
    } else {
      super.setDropListener(listener);
    }
  }

  final void setPlayitemStateSource(PlayitemStateSource source) {
    this.state = null != source ? source : new StubPlayitemStateSource();
  }

  final void load(LoaderManager manager) {
    manager.initLoader(LOADER_ID, null, this);
  }

  @Override
  public Loader<Cursor> onCreateLoader(int id, Bundle params) {
    getCursorAdapter().changeCursor(null);
    return new CursorLoader(getContext(), PlaylistQuery.ALL, null, null, null, null);
  }

  @Override
  public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
    getCursorAdapter().changeCursor(cursor);
    useStoredViewPosition();
  }

  @Override
  public void onLoaderReset(Loader<Cursor> loader) {
    getCursorAdapter().changeCursor(null);
  }

  private CursorAdapter getCursorAdapter() {
    return (CursorAdapter) getInputAdapter();
  }

  private class PlaylistCursorAdapter extends DragSortCursorAdapter {

    private DropListener dropListener;

    PlaylistCursorAdapter(Context context, @Nullable Cursor cursor, boolean autoRequery) {
      super(context, cursor, autoRequery);
    }

    PlaylistCursorAdapter(Context context, @Nullable Cursor cursor, int flags) {
      super(context, cursor, flags);
    }
    
    @Override
    public void bindView(View view, Context context, Cursor cursor) {
      final ListItemViewHolder holder = ListItemViewHolder.fromView(view);
      final Item item = new Item(cursor);
      final String title = item.getTitle();
      if (title.isEmpty()) {
        final String filename = item.getLocation().getDisplayFilename();
        holder.setMainText(filename);
      } else {
        holder.setMainText(title);
      }
      holder.setAuxText(item.getAuthor());
      holder.setDetailText(item.getDuration().toString());
      final Uri uri = item.getUri();
      final int icon = state.isPlaying(uri) ? R.drawable.ic_playing : R.drawable.ic_drag_handler;
      holder.setIcon(icon);
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      return ListItemViewHolder.createView(context, parent);
    }
    
    @Override
    public void drop(int from, int to) {
      //call listener first to get non-modified ids
      if (dropListener != null) {
        dropListener.drop(from, to);
      }
      super.drop(from, to);
    }
    
    final void setDropListener(DropListener listener) {
      dropListener = listener;
    }
  }

}
