/*
 * @file
 * 
 * @brief Playlist ui class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.annotation.TargetApi;
import android.app.Activity;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcelable;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playlist.Query;

public class PlaylistFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = PlaylistFragment.class.getName();
  private PlaybackService service;
  private Releaseable connection;
  private PlaylistState state;
  private PlaylistView listing;
  private volatile Uri nowPlaying = Uri.EMPTY;
  private volatile boolean isPlaying = false;
  private android.view.ActionMode actionMode;
  private android.support.v7.view.ActionMode actionModeCompat;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    
    state = new PlaylistState(PreferenceManager.getDefaultSharedPreferences(activity));
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.playlist, container, false) : null;
  }
  
  @Override
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setOnItemLongClickListener(new OnItemLongClickListener());
    listing.setPlayitemStateSource(new PlaylistView.PlayitemStateSource() {
      @Override
      public boolean isPlaying(Uri playlistUri) {
        return isPlaying && 0 == playlistUri.compareTo(nowPlaying);
      }
    });
    listing.setEmptyView(view.findViewById(R.id.playlist_stub));
    bindViewToConnectedService();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      listing.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE_MODAL);
      listing.setMultiChoiceModeListener(new MultiChoiceModeListener());
    }
  }
  
  @Override
  public void onViewStateRestored (Bundle savedInstanceState) {
    super.onViewStateRestored(savedInstanceState);

    if (savedInstanceState != null) {
      Log.d(TAG, "Loading operational state");
      final Parcelable state = savedInstanceState.getParcelable(listing.getClass().getName());
      listing.setTag(state);
    } else {
      Log.d(TAG, "Loading persistent state");
      listing.setTag(Integer.valueOf(state.getCurrentViewPosition()));
    }
    listing.load(getLoaderManager());
  }

  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);

    Log.d(TAG, "Saving operational state");
    final Parcelable state = listing.onSaveInstanceState();
    outState.putParcelable(listing.getClass().getName(), state);
  }
  
  @Override
  public synchronized void onDestroyView() {
    super.onDestroyView();

    Log.d(TAG, "Saving persistent state");
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
    unbindFromService();
  }
  
  @Override
  public void onServiceConnected(PlaybackService service) {
    this.service = service;
    
    bindViewToConnectedService();
  }
  
  private void bindViewToConnectedService() {
    final boolean serviceConnected = service != null;
    final boolean viewCreated = listing != null;
    if (serviceConnected && viewCreated) {
      Log.d(TAG, "Subscribe to service events");
      connection = new CallbackSubscription(service, new PlaybackCallback());
    }
  }
  
  private void unbindFromService() {
    try {
      if (connection != null) {
        Log.d(TAG, "Unsubscribe from service events");
        connection.release();
      }
    } finally {
      connection = null;
    }
    service = null;
  }
  
  private class OnItemClickListener implements PlaylistView.OnItemClickListener {

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
      if (inActionMode()) {
        updateActionModeTitle();
      } else {
        final Uri uri = Query.unparse(id);
        service.setNowPlaying(uri);
      }
    }
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
    updateActionModeTitle();
    return true;
  }
  
  private void updateActionModeTitle() {
    final int count = listing.getCheckedItemCount();
    final String title = getResources().getQuantityString(R.plurals.playlist_selecteditems, count, count);
    if (actionMode != null) {
      actionMode.setTitle(title);
    } else if (actionModeCompat != null) {
      actionModeCompat.setTitle(title);
    }
  }
  
  private boolean inActionMode() {
    return actionMode != null || actionModeCompat != null;
  }

  private class OnItemLongClickListener implements PlaylistView.OnItemLongClickListener {

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int pos, long id) {
      if (inActionMode()) {
        //TODO: dragging
        return false;
      } else {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
          final ActionBarActivity activity = (ActionBarActivity) getActivity();
          actionModeCompat = activity.startSupportActionMode(new ActionModeCallback());
          listing.performItemClick(view, pos, id);
        }
        return true;
      }
    }
  }
  
  private class ActionModeCallback implements android.support.v7.view.ActionMode.Callback {
    
    @Override
    public boolean onCreateActionMode(android.support.v7.view.ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.playlist, menu);
      listing.setChoiceMode(AbsListView.CHOICE_MODE_MULTIPLE);
      return true;
    }

    @Override
    public boolean onPrepareActionMode(android.support.v7.view.ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(android.support.v7.view.ActionMode mode, MenuItem item) {
      if (PlaylistFragment.this.onActionItemClicked(item)) {
        return true;
      } else if (item.getItemId() == R.id.action_delete) {
        service.getPlaylistControl().delete(listing.getCheckedItemIds());
        mode.finish();
        return true;
      } else {
        return false;
      }
    }

    @Override
    public void onDestroyActionMode(android.support.v7.view.ActionMode mode) {
      if (mode == actionModeCompat) {
        actionModeCompat = null;
        listing.selectNone();
        listing.postDelayed(new Runnable() {
          @Override
          public void run() {
            listing.setChoiceMode(AbsListView.CHOICE_MODE_NONE);
          }
        }, 200);
      }
    }
  }
  
  private class MultiChoiceModeListener implements AbsListView.MultiChoiceModeListener {

    @Override
    public boolean onCreateActionMode(android.view.ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.playlist, menu);
      actionMode = mode;
      return true;
    }

    @Override
    public boolean onPrepareActionMode(android.view.ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(android.view.ActionMode mode, MenuItem item) {
      if (PlaylistFragment.this.onActionItemClicked(item)) {
        return true;
      } else if (item.getItemId() == R.id.action_delete) {
        service.getPlaylistControl().delete(listing.getCheckedItemIds());
        mode.finish();
        return true;
      } else {
        return false;
      }
    }

    @Override
    public void onDestroyActionMode(android.view.ActionMode mode) {
      if (mode == actionMode) {
        actionMode = null;
      }
    }

    @Override
    public void onItemCheckedStateChanged(android.view.ActionMode mode, int position, long id,
        boolean checked) {
      updateActionModeTitle();
    }
    
  }
  
  private class PlaybackCallback implements Callback {
    
    private final Runnable updateTask;
    
    public PlaybackCallback() {
      this.updateTask = new Runnable() {
        @Override
        public void run() {
          listing.invalidateViews();
        }
      };
    }
    
    @Override
    public void onStatusChanged(boolean nowPlaying) {
      isPlaying = nowPlaying;
      updateView();
    }
    
    @Override
    public void onItemChanged(Item item) {
      nowPlaying = item != null ? item.getId() : Uri.EMPTY;
      updateView();
    }
    
    private void updateView() {
      listing.removeCallbacks(updateTask);
      listing.postDelayed(updateTask, 100);
    }
  }
}
