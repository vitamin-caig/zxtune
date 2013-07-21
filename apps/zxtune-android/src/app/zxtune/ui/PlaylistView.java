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
import app.zxtune.playlist.Item;
import app.zxtune.playlist.Query;

public class PlaylistView extends ListView
    implements
      AdapterView.OnItemClickListener,
      AdapterView.OnItemLongClickListener {

  public interface OnPlayitemClickListener {

    public void onPlayitemClick(Uri playlistUri);

    public boolean onPlayitemLongClick(Uri playlistUri);
  }

  public interface PlayitemStateSource {

    public boolean isPlaying(Uri playlistUri);
  }

  private static class StubOnPlayitemClickListener implements OnPlayitemClickListener {

    @Override
    public void onPlayitemClick(Uri playlistUri) {}

    @Override
    public boolean onPlayitemLongClick(Uri playlistUri) {
      return false;
    }
  }

  private static class StubPlayitemStateSource implements PlayitemStateSource{

    @Override
    public boolean isPlaying(Uri playlistUri) {
      return false;
    }
  }

  private PlayitemStateSource source;
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
  
  public void setPlayitemStateSource(PlayitemStateSource source) {
    this.source = null != source ? source : new StubPlayitemStateSource();
  }

  public void setData(Cursor cursor) {
    final CursorAdapter adapter = new PlaylistCursorAdapter(getContext(), cursor, true);
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

  private class PlaylistCursorAdapter extends CursorAdapter {

    private final LayoutInflater inflater;

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
      final ViewHolder holder = (ViewHolder) view.getTag();
      final Item item = new Item(cursor);
      if (0 == item.getTitle().length()) {
        holder.title.setText(item.getLocation().toString());
      } else {
        holder.title.setText(item.getTitle());
      }
      holder.author.setText(item.getAuthor());
      holder.type.setText(item.getType());
      holder.duration.setText(item.getDuration().toString());
      view.setBackgroundResource(source.isPlaying(item.getUri()) ? R.drawable.ic_playing : 0);
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
    final TextView type;
    
    public ViewHolder(View view) {
      this.title = (TextView) view.findViewById(R.id.playlist_item_title);
      this.author = (TextView) view.findViewById(R.id.playlist_item_author);
      this.duration = (TextView) view.findViewById(R.id.playlist_item_duration);
      this.type = (TextView) view.findViewById(R.id.playlist_item_type);
    }
  }
}
