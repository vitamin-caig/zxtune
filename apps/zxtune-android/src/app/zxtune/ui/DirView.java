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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
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
  
  private static class Tags {

    static final String ICON = "icon";
    static final String NAME = "name";
    static final String SIZE = "size";
    static final String URI = "uri";
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
      listener.onFileClick(item.getUri());
    } else {
      listener.onDirClick(item.getUri());
    }
  }

  @Override
  public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
    final ItemData item = (ItemData) parent.getAdapter().getItem(position);
    if (item.isFile()) {
      return listener.onFileLongClick(item.getUri());
    } else {
      return listener.onDirLongClick(item.getUri());
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
      final List<Map<String, Object>> stubData = Collections.emptyList();
      setData(stubData);
    } else {
      Arrays.sort(entries, new CompareEntries());
      final List<Map<String, Object>> data = new DelayedListViewItems(entries);
      setData(data);
    }
  }

  private final void setData(List<Map<String, Object>> data) {
    final String[] fromFields = {Tags.ICON, Tags.NAME, Tags.SIZE};
    final int[] toFields = {R.id.dirview_item_icon, R.id.dirview_item_name, R.id.dirview_item_size};

    final SimpleAdapter adapter =
        new SimpleAdapter(getContext(), data, R.layout.dirview_item, fromFields, toFields);
    setAdapter(adapter);
    adapter.notifyDataSetChanged();
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

  private static class DelayedListViewItems extends AbstractList<Map<String, Object>> {

    private final Object[] content;

    public DelayedListViewItems(Vfs.Entry[] entries) {
      content = new Object[entries.length];
      System.arraycopy(entries, 0, content, 0, entries.length);
    }

    @Override
    public Map<String, Object> get(int index) {
      final Object val = content[index];
      if (val instanceof Map<?, ?>) {
        return (Map<String, Object>) val;
      } else {
        final ItemData res = createItemData((Vfs.Entry) content[index]);
        content[index] = res;
        return res;
      }
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

  private static class ItemData extends HashMap<String, Object> {

    private static final long serialVersionUID = 1L;

    public ItemData(Vfs.Dir dir) {
      put(Tags.ICON, R.drawable.ic_browser_folder);
      put(Tags.NAME, dir.name());
      put(Tags.URI, dir.uri());
    }

    public ItemData(Vfs.File file) {
      put(Tags.ICON, R.drawable.ic_browser_file);
      put(Tags.NAME, file.name());
      put(Tags.SIZE, file.size());
      put(Tags.URI, file.uri());
    }

    public final boolean isFile() {
      return containsKey(Tags.SIZE);
    }

    public final Uri getUri() {
      return (Uri) get(Tags.URI);
    }
  }
}
