/*
 * @file
 * @brief Browser ui class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListAdapter;
import app.zxtune.MainService;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.fs.VfsQuery;
import app.zxtune.playback.PlaybackService;

public class BrowserFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = BrowserFragment.class.getName();
  private PlaybackService service;
  private BrowserState state;
  private View sources;
  private BreadCrumbsUriView position;
  private DirView listing;

  public static Fragment createInstance() {
    return new BrowserFragment();
  }

  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    
    state = new BrowserState(PreferenceManager.getDefaultSharedPreferences(activity));
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.browser, container, false) : null;
  }

  @Override
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    
    sources = view.findViewById(R.id.browser_sources);
    sources.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        setCurrentPath(Uri.EMPTY);
      }
    });
    position = (BreadCrumbsUriView) view.findViewById(R.id.browser_breadcrumb);
    position.setOnUriSelectionListener(new BreadCrumbsUriView.OnUriSelectionListener() {
      @Override
      public void onUriSelection(Uri uri) {
        setCurrentPath(uri);
      }
    });
    listing = (DirView) view.findViewById(R.id.browser_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setEmptyView(view.findViewById(R.id.browser_stub));
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      listing.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE_MODAL);
      listing.setMultiChoiceModeListener(new MultiChoiceModeListener());
    }
    if (savedInstanceState == null) {
      Log.d(TAG, "Loading persistent state");
      listing.setTag(state);
    }
    position.setUri(state.getCurrentPath());
    listing.load(getLoaderManager());
  }

  @Override
  public synchronized void onDestroyView() {
    super.onDestroyView();

    Log.d(TAG, "Saving persistent state");
    storeCurrentViewPosition();
    service = null;
  }

  @Override
  public void onServiceConnected(PlaybackService service) {
    this.service = service;
  }
  
  private String getActionModeTitle() {
    final int count = listing.getCheckedItemCount();
    return getResources().getQuantityString(R.plurals.selected_items, count, count);
  }

  private boolean onActionItemClicked(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.action_select_all:
        listing.selectAll();
        break;
      case R.id.action_select_none:
        listing.selectNone();
        break;
      case R.id.action_select_invert:
        listing.invertSelection();
        break;
      default:
        return false;
    }
    return true;
  }

  class OnItemClickListener implements AdapterView.OnItemClickListener {

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
      final Cursor cursor = (Cursor) parent.getAdapter().getItem(position);
      final Uri uri = Uri.parse(cursor.getString(VfsQuery.Columns.URI));
      if (VfsQuery.Types.FILE == cursor.getInt(VfsQuery.Columns.TYPE)) {
        onFileClick(uri);
      } else {
        onDirClick(uri);
      }
    }

    private void onFileClick(Uri uri) {
      final Uri[] toPlay = {uri};
      service.setNowPlaying(toPlay);
    }

    private void onDirClick(Uri uri) {
      setCurrentPath(uri);
    }
  }

  private class MultiChoiceModeListener implements AbsListView.MultiChoiceModeListener {

    @Override
    public boolean onCreateActionMode(android.view.ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.browser, menu);
      mode.setTitle(getActionModeTitle());
      return true;
    }

    @Override
    public boolean onPrepareActionMode(android.view.ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(android.view.ActionMode mode, MenuItem item) {
      if (BrowserFragment.this.onActionItemClicked(item)) {
        return true;
      } else {
        switch (item.getItemId()) {
          case R.id.action_play:
            service.setNowPlaying(getSelectedItemsUris());
            break;
          case R.id.action_add:
            service.getPlaylistControl().add(getSelectedItemsUris());
            break;
          default:
            return false;
        }
        mode.finish();
        return true;
      }
    }

    @Override
    public void onDestroyActionMode(android.view.ActionMode mode) {}

    @Override
    public void onItemCheckedStateChanged(android.view.ActionMode mode, int position, long id,
        boolean checked) {
      mode.setTitle(getActionModeTitle());
    }
    
    private Uri[] getSelectedItemsUris() {
      final Uri[] result = new Uri[listing.getCheckedItemCount()];
      final SparseBooleanArray selected = listing.getCheckedItemPositions();
      final ListAdapter adapter = listing.getAdapter();
      int pos = 0;
      for (int i = 0, lim = selected.size(); i != lim; ++i) {
        if (selected.valueAt(i)) {
          final Cursor cursor = (Cursor) adapter.getItem(selected.keyAt(i));
          result[pos++] = Uri.parse(cursor.getString(VfsQuery.Columns.URI));
        }
      }
      return result;
    }
  }

  private final void setCurrentPath(Uri uri) {
    storeCurrentViewPosition();
    setNewState(uri);
  }

  private final void storeCurrentViewPosition() {
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }

  private final void setNewState(Uri uri) {
    Log.d(TAG, "Set current path to " + uri);
    state.setCurrentPath(uri);
    position.setUri(uri);
    listing.setTag(state);
    listing.load(getLoaderManager());
  }
}
