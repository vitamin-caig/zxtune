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
import android.support.v4.widget.CursorAdapter;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.playlist.Query;
import app.zxtune.playlist.Item;

public class PlaylistView extends ListView
    implements
      AdapterView.OnItemClickListener,
      AdapterView.OnItemLongClickListener {

  public interface OnPlayitemClickListener {

    public void onPlayitemClick(Uri playlistUri);

    public boolean onPlayitemLongClick(Uri playlistUri);
  }

  public static class StubOnPlayitemClickListener implements OnPlayitemClickListener {

    @Override
    public void onPlayitemClick(Uri playlistUri) {}

    @Override
    public boolean onPlayitemLongClick(Uri playlistUri) {
      return false;
    }
  }

  private OnPlayitemClickListener listener;

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
    listener = new StubOnPlayitemClickListener();
    super.setLongClickable(true);
    super.setOnItemClickListener(this);
    super.setOnItemLongClickListener(this);
  }

  public void setOnPlayitemClickListener(OnPlayitemClickListener listener) {
    this.listener = null != listener ? listener : new StubOnPlayitemClickListener();
  }

  public void setData(Cursor cursor) {
    final CursorAdapter adapter = new PlaylistCursorAdapter(getContext(), cursor);
    this.setAdapter(adapter);
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
    listener.onPlayitemClick(Query.unparse(id));
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    return listener.onPlayitemLongClick(Query.unparse(id));
  }

  private static class PlaylistCursorAdapter extends CursorAdapter {

    private final LayoutInflater inflater;

    public PlaylistCursorAdapter(Context context, Cursor cursor) {
      super(context, cursor);
      inflater = LayoutInflater.from(context);
    }

    public PlaylistCursorAdapter(Context context, Cursor cursor, boolean autoRequery) {
      super(context, cursor, autoRequery);
      inflater = LayoutInflater.from(context);
    }

    public PlaylistCursorAdapter(Context context, Cursor cursor, int flags) {
      super(context, cursor, flags);
      inflater = LayoutInflater.from(context);
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
      final Item item = new Item(cursor);
      bindType(item, view);
      bindTitle(item, view);
      bindDuration(item, view);
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      return inflater.inflate(R.layout.playlist_item, parent, false);
    }

    private static void bindType(Item item, View view) {
      final TextView type = (TextView) view.findViewById(R.id.playlist_item_type);
      type.setText(item.getType());
    }

    private static void bindTitle(Item item, View view) {
      final TextView title = (TextView) view.findViewById(R.id.playlist_item_title);
      final TextView author = (TextView) view.findViewById(R.id.playlist_item_author);
      title.setText(item.getTitle());
      if (0 == item.getAuthor().length()) {
        author.setVisibility(View.GONE);
      } else {
        author.setText(item.getAuthor());
        author.setVisibility(View.VISIBLE);
      }
    }

    private static void bindDuration(Item item, View view) {
      final TextView duration = (TextView) view.findViewById(R.id.playlist_item_duration);
      duration.setText(item.getDuration());
    }
  }
}
