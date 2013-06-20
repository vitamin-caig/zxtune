/*
 * @file
 * 
 * @brief DirView class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.AbstractList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.Vfs;

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
    final ItemData item = (ItemData) parent.getAdapter().getItem(position);
    if (item.isFile()) {
      listener.onFileClick(item.uri);
    } else {
      listener.onDirClick(item.uri);
    }
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    final ItemData item = (ItemData) parent.getAdapter().getItem(position);
    if (item.isFile()) {
      return listener.onFileLongClick(item.uri);
    } else {
      return listener.onDirLongClick(item.uri);
    }
  }

  public void setOnEntryClickListener(OnEntryClickListener listener) {
    this.listener = null != listener ? listener : new StubOnEntryClickListener();
  }

  public void setUri(Uri path) {
    setDir((Vfs.Dir) Vfs.getRoot().resolve(path));
  }

  public void setDir(Vfs.Dir dir) {

    final Vfs.Entry[] entries = dir != null ? dir.list() : null;
    if (entries == null) {
      final List<ItemData> stubData = Collections.emptyList();
      setData(stubData);
    } else {
      Arrays.sort(entries, new CompareEntries());
      final List<ItemData> data = new DelayedListViewItems(entries);
      setData(data);
    }
  }

  private final void setData(List<ItemData> data) {
    final ListAdapter adapter = new DirViewAdapter(getContext(), data);
    setAdapter(adapter);
  }

  private static class CompareEntries implements Comparator<Vfs.Entry> {

    @Override
    public int compare(Vfs.Entry lh, Vfs.Entry rh) {
      final int byType = compareByType(lh, rh);
      return byType != 0 ? byType : compareByName(lh, rh);
    }

    private static int compareByType(Vfs.Entry lh, Vfs.Entry rh) {
      final int lhDir = lh instanceof Vfs.Dir ? 1 : 0;
      final int rhDir = rh instanceof Vfs.Dir ? 1 : 0;
      return rhDir - lhDir;
    }

    private static int compareByName(Vfs.Entry lh, Vfs.Entry rh) {
      return lh.name().compareToIgnoreCase(rh.name());
    }
  }
  
  private static class DirViewAdapter extends BaseAdapter {
    
    private final LayoutInflater inflater;
    private final List<ItemData> items;
    
    public DirViewAdapter(Context context, List<ItemData> items) {
      this.inflater = LayoutInflater.from(context);
      this.items = items;
    }
    
    @Override
    public int getCount() {
      return items.size();
    }
    
    @Override
    public ItemData getItem(int position) {
      return items.get(position);
    }
    
    @Override
    public long getItemId(int position) {
      return position;
    }
    
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
      ViewHolder holder;
      if (convertView != null) {
        holder = (ViewHolder) convertView.getTag();
      } else {
        convertView = inflater.inflate(R.layout.dirview_item, null);
        holder = new ViewHolder();
        holder.icon = (ImageView) convertView.findViewById(R.id.dirview_item_icon);
        holder.name = (TextView) convertView.findViewById(R.id.dirview_item_name);
        holder.size = (TextView) convertView.findViewById(R.id.dirview_item_size);
        convertView.setTag(holder);
      }
      final ItemData item = getItem(position);
      if (item.isFile()) {
        holder.icon.setVisibility(GONE);
        holder.size.setVisibility(VISIBLE);
        holder.size.setText(item.size.toString());
      } else {
        holder.icon.setVisibility(VISIBLE);
        holder.size.setVisibility(GONE);
      }
      holder.name.setText(item.name);
      return convertView;
    }
    
    private static class ViewHolder {
      public ImageView icon;
      public TextView name;
      public TextView size;
    }
  }

  private static class DelayedListViewItems extends AbstractList<ItemData> {

    private Vfs.Entry[] entries;
    private final ItemData[] content;
    private int cachedCount;

    public DelayedListViewItems(Vfs.Entry[] entries) {
      this.entries = entries;
      this.content = new ItemData[entries.length];
      this.cachedCount = 0;
    }

    @Override
    public ItemData get(int index) {
      ItemData val = content[index];
      if (val == null) {
        val = createItemData(entries[index]);
        content[index] = val;
        if (++cachedCount == entries.length) {
          entries = null;
        }
      }
      return val;
    }

    @Override
    public int size() {
      return content.length;
    }

    private static ItemData createItemData(Vfs.Entry entry) {
      if (entry instanceof Vfs.Dir) {
        return new ItemData((Vfs.Dir) entry);
      } else if (entry instanceof Vfs.File) {
        return new ItemData((Vfs.File) entry);
      } else {
        throw new UnsupportedOperationException();
      }
    }
  }

  private static class ItemData {

    public final String name;
    public final Long size;
    public final Uri uri;

    public ItemData(Vfs.Dir dir) {
      this.name = dir.name();
      this.size = null;
      this.uri = dir.uri();
    }

    public ItemData(Vfs.File file) {
      this.name = file.name();
      this.size = file.size();
      this.uri = file.uri();
    }

    public final boolean isFile() {
      return size != null;
    }
  }
}
