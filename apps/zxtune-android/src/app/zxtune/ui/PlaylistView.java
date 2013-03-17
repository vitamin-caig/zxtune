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
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;

public class PlaylistView extends ListView
    implements
      AdapterView.OnItemClickListener,
      AdapterView.OnItemLongClickListener {

  public interface OnPlayitemClickListener {

    public void onPlayitemClick(long id, String dataUri);

    public boolean onPlayitemLongClick(long id, String dataUri);
  }

  public static class StubOnPlayitemClickListener implements OnPlayitemClickListener {

    @Override
    public void onPlayitemClick(long id, String dataUri) {}

    @Override
    public boolean onPlayitemLongClick(long id, String dataUri) {
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
    final CursorAdapter adapter =
        new PlaylistCursorAdapter(getContext(), cursor, CursorAdapter.FLAG_AUTO_REQUERY);
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
    final String uri = (String) view.getTag();
    listener.onPlayitemClick(id, uri);
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    final String uri = (String) view.getTag();
    return listener.onPlayitemLongClick(id, uri);
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
      bindUri(cursor, view);
      bindType(cursor, view);
      bindTitle(cursor, view);
      bindDuration(cursor, view);
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      return inflater.inflate(R.layout.playlist_item, parent, false);
    }

    private static void bindUri(Cursor cursor, View view) {
      final String uri = cursor.getString(1);//TODO
      view.setTag(uri);
    }

    private static void bindType(Cursor cursor, View view) {
      final String itemType = cursor.getString(2);
      final ImageView icon = (ImageView) view.findViewById(R.id.playlist_item_typeicon);
      icon.setImageResource(getTypeResourceIcon(itemType));
    }

    private static int getTypeResourceIcon(String type) {
      return R.drawable.ic_play;//TODO
    }

    private static void bindTitle(Cursor cursor, View view) {
      final String itemAuthor = cursor.getString(3);//TODO
      final String itemTitle = cursor.getString(4);//TODO
      final String itemDisplayTitle =
          0 == itemAuthor.length() + itemAuthor.length() ? cursor.getString(1)//TODO
              : getDisplayTitle(itemAuthor, itemTitle);
      final TextView displayTitle = (TextView) view.findViewById(R.id.playlist_item_displaytitle);
      displayTitle.setText(itemDisplayTitle);
    }

    private static String getDisplayTitle(String author, String title) {
      if (0 == author.length()) {
        author = "Unknown author";//TODO
      }
      if (0 == title.length()) {
        title = "Unknown title";//TODO
      }
      return author + " - " + title;
    }

    private static void bindDuration(Cursor cursor, View view) {
      final int durationMs = cursor.getInt(5);//TODO
      final TextView duration = (TextView) view.findViewById(R.id.playlist_item_duration);
      duration.setText(String.valueOf(durationMs / 1000));//TODO
    }
  }
}
