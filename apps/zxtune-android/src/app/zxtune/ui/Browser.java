/*
 * @file
 * 
 * @brief Browser ui class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import android.content.Context;
import android.database.DataSetObserver;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.Vfs;

public class Browser extends Fragment {

  public static interface Callback {
    
    public void onFileSelected(Uri uri);
  }

  private final Context context;
  private final Callback callback;
  private final Vfs.Dir roots;

  private ListView currentView;
  private TextView currentPath;
  private Vfs.Dir current;

  private static class Columns {

    static final String ICON = "icon";
    static final String NAME = "name";
    static final String SIZE = "size";
  }

  public Browser(Context context, Callback callback) {
    this.context = context;
    this.callback = callback;
    this.roots = Vfs.getRoot();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.browser, null);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    final Spinner rootsList = (Spinner) view.findViewById(R.id.browser_roots);
    final Vfs.Entry[] allRoots = roots.list();
    rootsList.setAdapter(createViewAdapter(context, allRoots));
    rootsList.setOnItemSelectedListener(new ItemSelectionListener(allRoots));
    currentPath = (TextView) view.findViewById(R.id.browser_position);
    currentView = (ListView) view.findViewById(R.id.browser_content);
    navigate(roots);
  }

  private static SimpleAdapter createViewAdapter(Context context, Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = new ArrayList<Map<String, Object>>(entries.length);

    for (Vfs.Entry entry : entries) {
      final Map<String, Object> item = createItemData(entry);
      data.add(item);
    }

    final String[] fromFields = {Columns.ICON, Columns.NAME, Columns.SIZE};
    final int[] toFields = {R.id.browser_item_icon, R.id.browser_item_name, R.id.browser_item_size};

    return new SimpleAdapter(context, data, R.layout.browser_item, fromFields, toFields);
  }

  private static Map<String, Object> createItemData(Vfs.Entry entry) {

    if (entry instanceof Vfs.Dir) {
      return createItemData((Vfs.Dir) entry);
    } else if (entry instanceof Vfs.File) {
      return createItemData((Vfs.File) entry);
    } else {
      return null;
    }
  }

  private static Map<String, Object> createItemData(Vfs.Dir file) {

    final Map<String, Object> result = new HashMap<String, Object>();
    result.put(Columns.ICON, R.drawable.ic_browser_folder);
    result.put(Columns.NAME, file.name());
    return result;
  }

  private static Map<String, Object> createItemData(Vfs.File file) {

    final Map<String, Object> result = new HashMap<String, Object>();
    result.put(Columns.ICON, R.drawable.ic_browser_file);
    result.put(Columns.NAME, file.name());
    result.put(Columns.SIZE, file.size());
    return result;
  }

  private void navigate(Vfs.Entry entry) {
    if (entry instanceof Vfs.Dir) {
      navigate((Vfs.Dir) entry);
    } else if (entry instanceof Vfs.File) {
      callback.onFileSelected(entry.uri());
    }
  }

  private void navigate(Vfs.Dir dir) {
    current = dir;
    final Uri curUri = current.uri();
    if (curUri != null) {
      currentPath.setText(curUri.toString());
    }
    final Vfs.Entry[] entries = current.list();
    final SimpleAdapter adapter = createViewAdapter(context, entries);
    currentView.setAdapter(adapter);
    currentView.setOnItemClickListener(new ItemClickListener(entries));
    adapter.notifyDataSetChanged();
  }

  private class ItemSelectionListener implements AdapterView.OnItemSelectedListener {

    final Vfs.Entry[] entries;

    ItemSelectionListener(Vfs.Entry[] entries) {
      this.entries = entries;
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
      navigate(entries[position]);
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {

    }
  }

  private class ItemClickListener implements AdapterView.OnItemClickListener {

    final Vfs.Entry[] entries;

    ItemClickListener(Vfs.Entry[] entries) {
      this.entries = entries;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
      navigate(entries[position]);
    }
  }
}
