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
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.database.DataSetObserver;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
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

  private ListView listing;
  private BreadCrumbNavigation position;

  private static final String TAG = "app.zxtune.ui.Browser";
  
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
    rootsList.setAdapter(createSpinnerAdapter(context, allRoots));
    rootsList.setOnItemSelectedListener(new ItemSelectionListener(allRoots));
    position = new BreadCrumbNavigation((HorizontalScrollView) view.findViewById(R.id.browser_breadcrumb));
    listing = (ListView) view.findViewById(R.id.browser_content);
  }

  private static SimpleAdapter createSpinnerAdapter(Context context, Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = createItemsData(entries);

    final String[] fromFields = {Columns.NAME};
    final int[] toFields = {R.id.browser_item_name};

    return new SimpleAdapter(context, data, R.layout.browser_item, fromFields, toFields);
  }

  private static SimpleAdapter createListViewAdapter(Context context, Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = createItemsData(entries);

    final String[] fromFields = {Columns.ICON, Columns.NAME, Columns.SIZE};
    final int[] toFields = {R.id.browser_item_icon, R.id.browser_item_name, R.id.browser_item_size};

    return new SimpleAdapter(context, data, R.layout.browser_item, fromFields, toFields);
  }

  private static ArrayList<Map<String, Object>> createItemsData(Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = new ArrayList<Map<String, Object>>(entries.length);

    for (Vfs.Entry entry : entries) {
      final Map<String, Object> item = createItemData(entry);
      data.add(item);
    }
    return data;
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

  private final void navigate(Vfs.Entry entry) {
    if (entry instanceof Vfs.Dir) {
      navigate((Vfs.Dir) entry);
    } else if (entry instanceof Vfs.File) {
      callback.onFileSelected(entry.uri());
    }
  }

  private final void navigate(Vfs.Dir dir) {
    final Uri uri = dir.uri();
    final Vfs.Entry[] entries = dir.list();
    if (entries == null) {
      // showAlert();
    } else {
      setCurrentPath(uri);
      setCurrentEntries(entries);
    }
  }

  private final void navigate(Uri uri) {
    navigate(roots.resolve(uri));
  }

  private final void setCurrentPath(Uri uri) {
    Log.d(TAG, "Set current path to " + uri); 
    if (uri != null) {
      position.setPath(uri);
    }
  }

  private final void setCurrentEntries(Vfs.Entry[] entries) {
    Arrays.sort(entries, new CompareEntries());
    final SimpleAdapter adapter = createListViewAdapter(context, entries);
    listing.setAdapter(adapter);
    listing.setOnItemClickListener(new ItemClickListener(entries));
    adapter.notifyDataSetChanged();
  }

  private static class CompareEntries implements Comparator<Vfs.Entry> {

    @Override
    public int compare(Vfs.Entry lh, Vfs.Entry rh) {
      final boolean lhDir = lh instanceof Vfs.Dir;
      final boolean rhDir = rh instanceof Vfs.Dir;
      if (lhDir == rhDir) {
        return lh.name().compareTo(rh.name());
      } else {
        return lhDir ? -1 : +1;
      }
    }
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

  private class PathElementClickListener implements View.OnClickListener {

    final Uri uri;

    PathElementClickListener(Uri uri) {
      this.uri = uri;
    }

    @Override
    public void onClick(View v) {
      navigate(uri);
    }
  }

  private class BreadCrumbNavigation {

    private final HorizontalScrollView scroll;
    private final ViewGroup container;
    private final ArrayList<Button> buttons;

    public BreadCrumbNavigation(HorizontalScrollView scroll) {
      this.scroll = scroll;
      this.container = new LinearLayout(context);
      this.buttons = new ArrayList<Button>();
      scroll.addView(container);
    }

    public void setPath(Uri uri) {
      final List<String> elements = getPathElements(uri); 
      final int newElements = elements.size();
      final int curElements = buttons.size();
      final int toUpdate = Math.min(newElements, curElements);
      final Uri.Builder subUri = uri.buildUpon();
      subUri.path("");
      for (int idx = 0; idx != toUpdate; ++idx) {
        final String pathElement = elements.get(idx);
        subUri.appendPath(pathElement);
        buttons.get(idx).setText(pathElement);
      }
      for (int toAdd = curElements; toAdd < newElements; ++toAdd) {
        final String pathElement = elements.get(toAdd);
        subUri.appendPath(pathElement);
        final Button button = new Button(context);
        button.setText(pathElement);
        button.setOnClickListener(new PathElementClickListener(subUri.build()));
        buttons.add(button);
        container.addView(button);
      }
      for (int toDel = curElements; toDel > newElements; --toDel) {
        buttons.remove(toDel - 1);
        container.removeViewAt(toDel - 1);
      }
      //workaround for layout timing issues
      scroll.post(new Runnable() {
        @Override
        public void run() {
          scroll.smoothScrollTo(Integer.MAX_VALUE, 0);
        }
      });
    }
    
    private final List<String> getPathElements(Uri uri) {
      final List<String> elements = new ArrayList<String>(); 
      elements.add("/");
      elements.addAll(uri.getPathSegments());
      return elements;
    }
  }
}
