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
import java.util.Map;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.SimpleAdapter;
import app.zxtune.PlaybackService;
import app.zxtune.R;
import app.zxtune.fs.Vfs;

public class Browser extends Fragment implements BreadCrumbsUriView.OnUriSelectionListener {

  private Context context;
  private final Vfs.Dir roots;

  private Button sources;
  private BreadCrumbsUriView position;
  private ListView listing;
  private Uri current;

  private static class Tags {

    static final String LOG = "app.zxtune.ui.Browser";
    static final String ICON = "icon";
    static final String NAME = "name";
    static final String SIZE = "size";
    static final String PATH = "path";
  }

  public Browser() {
    this.roots = Vfs.getRoot();
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    this.context = activity;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.browser, null);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    sources = (Button) view.findViewById(R.id.browser_sources);
    sources.setOnClickListener(new SourcesClickListener());
    position = (BreadCrumbsUriView) view.findViewById(R.id.browser_breadcrumb);
    position.setOnUriSelectionListener(this);
    listing = (ListView) view.findViewById(R.id.browser_content);
  }
  
  @Override
  public void onViewStateRestored(Bundle inState) {
    super.onViewStateRestored(inState);
    final Uri lastPath = inState != null ? (Uri) inState.getParcelable(Tags.PATH) : Uri.EMPTY;
    if (lastPath != Uri.EMPTY) {
      Log.d(Tags.LOG, "Restore position: " + lastPath);
      onUriSelection(lastPath);
    }
  }
  
  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    Log.d(Tags.LOG, "Store position: " + current);
    outState.putParcelable(Tags.PATH, current);
  }

  private static SimpleAdapter createSpinnerAdapter(Context context, Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = createItemsData(entries);

    final String[] fromFields = {Tags.ICON, Tags.NAME};
    final int[] toFields = {R.id.browser_item_icon, R.id.browser_item_name};

    return new SimpleAdapter(context, data, R.layout.browser_item, fromFields, toFields);
  }

  private static SimpleAdapter createListViewAdapter(Context context, Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = createItemsData(entries);

    final String[] fromFields = {Tags.ICON, Tags.NAME, Tags.SIZE};
    final int[] toFields = {R.id.browser_item_icon, R.id.browser_item_name, R.id.browser_item_size};

    return new SimpleAdapter(context, data, R.layout.browser_item, fromFields, toFields);
  }

  private static ArrayList<Map<String, Object>> createItemsData(Vfs.Entry[] entries) {

    final ArrayList<Map<String, Object>> data = new ArrayList<Map<String, Object>>(entries.length);

    for (Vfs.Entry entry : entries) {
      final Map<String, Object> item = createItemData(entry);
      if (item != null) {
        data.add(item);
      }
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

  private static Map<String, Object> createItemData(Vfs.Dir dir) {

    final Map<String, Object> result = new HashMap<String, Object>();
    result.put(Tags.ICON, R.drawable.ic_browser_folder);
    result.put(Tags.NAME, dir.name());
    return result;
  }

  private static Map<String, Object> createItemData(Vfs.File file) {

    final Map<String, Object> result = new HashMap<String, Object>();
    result.put(Tags.ICON, R.drawable.ic_browser_file);
    result.put(Tags.NAME, file.name());
    result.put(Tags.SIZE, file.size());
    return result;
  }

  private final void fileSelected(Uri uri) {
    final Intent intent = new Intent(Intent.ACTION_VIEW, uri, context, PlaybackService.class);
    context.startService(intent);
  }
  
  private final void navigate(Vfs.Entry entry) {
    if (entry instanceof Vfs.Dir) {
      navigate((Vfs.Dir) entry);
    } else if (entry instanceof Vfs.File) {
      fileSelected(entry.uri());
    }
  }
  
  private final void navigate(Vfs.Dir dir) {
    final Uri uri = dir.uri();
    final Vfs.Entry[] entries = dir.list();
    if (entries == null) {
      Log.d(Tags.LOG, "Unable to read entries for " + uri);
      final Vfs.Entry[] NO_ENTRIES = {};
      setCurrentPath(uri);
      setCurrentEntries(NO_ENTRIES);
    } else {
      setCurrentPath(uri);
      setCurrentEntries(entries);
    }
  }

  public void onUriSelection(Uri uri) {
    navigate(roots.resolve(uri));
  }

  private final void setCurrentPath(Uri uri) {
    Log.d(Tags.LOG, "Set current path to " + uri);
    current = uri;
    position.setUri(uri);
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
        return lh.name().compareToIgnoreCase(rh.name());
      } else {
        return lhDir ? -1 : +1;
      }
    }
  }

  private class SourcesClickListener implements View.OnClickListener {

    @Override
    public void onClick(View v) {
      final Vfs.Entry[] entries = roots.list();
      final SimpleAdapter adapter = createSpinnerAdapter(context, entries);
      final ListView view = new ListView(v.getContext());
      view.setAdapter(adapter);
      
      final Context context = v.getContext();
      final View root = view.inflate(context, R.layout.popup, null);
      final ViewGroup rootLayout = (ViewGroup) root.findViewById(R.id.popup_layout);
      rootLayout.addView(view);

      final PopupWindow popup = new PopupWindow(root, WindowManager.LayoutParams.WRAP_CONTENT, WindowManager.LayoutParams.WRAP_CONTENT, true);
      popup.setBackgroundDrawable(new BitmapDrawable());
      popup.setTouchable(true);
      popup.setOutsideTouchable(true);
      
      view.setOnItemClickListener(new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
          popup.dismiss();
          navigate(entries[position]);
        }
      });
      popup.showAsDropDown(v);
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
