/**
 *
 * @file
 *
 * @brief Playlist fragment component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.sax.TextElementListener;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.Toast;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playlist.PlaylistQuery;
import app.zxtune.playlist.XspfStorage;

public class PlaylistFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = PlaylistFragment.class.getName();
  private PlaybackService service;
  private Releaseable connection;
  private PlaylistState state;
  private PlaylistView listing;
  private MenuItem addNowPlaying;
  private final NowPlayingState playingState;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }
  
  public PlaylistFragment() {
    this.playingState = new NowPlayingState();
    setHasOptionsMenu(true);
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    
    state = new PlaylistState(PreferenceManager.getDefaultSharedPreferences(activity));
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setHasOptionsMenu(true);
  }
  
  @Override
  public void onCreateOptionsMenu (Menu menu, MenuInflater inflater) {
    super.onCreateOptionsMenu(menu, inflater);

    inflater.inflate(R.menu.playlist, menu);
    addNowPlaying = menu.findItem(R.id.action_add_current);
  }
  
  @Override
  public boolean onOptionsItemSelected (MenuItem item) {
    switch (item.getItemId()) {
      case R.id.action_clear:
        service.getPlaylistControl().deleteAll();
        break;
      case R.id.action_add_current:
        service.getPlaylistControl().add(new Uri[] {service.getNowPlaying().getDataId()});
        //disable further addings
        addNowPlaying.setVisible(false);
        break;
      case R.id.action_save:
        PlaylistSaveFragment.createInstance().show(getFragmentManager(), "save");
        break;
      default:
        return super.onOptionsItemSelected(item);
    }
    return true;
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.playlist, container, false) : null;
  }
  
  @Override
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setPlayitemStateSource(playingState);
    listing.setEmptyView(view.findViewById(R.id.playlist_stub));
    listing.setMultiChoiceModeListener(new MultiChoiceModeListener());
    bindViewToConnectedService();
    if (savedInstanceState == null) {
      Log.d(TAG, "Loading persistent state");
      listing.setTag(Integer.valueOf(state.getCurrentViewPosition()));
    }
    listing.load(getLoaderManager());
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
      connection = new CallbackSubscription(service, playingState);
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
      final Uri[] toPlay = {PlaylistQuery.uriFor(id)};
      service.setNowPlaying(toPlay);
    }
  }
  
  private class NowPlayingState implements Callback, PlaylistView.PlayitemStateSource {
    
    private final Runnable updateTask;
    private boolean isPlaying;
    private Uri nowPlayingPlaylist;
    
    public NowPlayingState() {
      this.updateTask = new Runnable() {
        @Override
        public void run() {
          listing.invalidateViews();
          addNowPlaying.setVisible(isPlaying && nowPlayingPlaylist == Uri.EMPTY);
        }
      };
      isPlaying = false;
      nowPlayingPlaylist = Uri.EMPTY;
    }
    
    @Override
    public void onStatusChanged(boolean nowPlaying) {
      isPlaying = nowPlaying;
      updateView();
    }
    
    @Override
    public void onItemChanged(Item item) {
      final Uri id = item.getId();
      final Uri contentId = item.getDataId();
      nowPlayingPlaylist = id.equals(contentId) ? Uri.EMPTY : id;
      updateView();
    }

    @Override
    public boolean isPlaying(Uri playlistUri) {
      return isPlaying && 0 == playlistUri.compareTo(nowPlayingPlaylist);
    }
    
    private void updateView() {
      listing.removeCallbacks(updateTask);
      listing.postDelayed(updateTask, 100);
    }
  }
  
  private String getActionModeTitle() {
    final int count = listing.getCheckedItemsCount();
    return getResources().getQuantityString(R.plurals.tracks, count, count);
  }
  
  private class MultiChoiceModeListener implements CheckableListView.MultiChoiceModeListener {

    @Override
    public boolean onCreateActionMode(CheckableListView.ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.selection, menu);
      inflater.inflate(R.menu.playlist_items, menu);
      mode.setTitle(getActionModeTitle());
      return true;
    }

    @Override
    public boolean onPrepareActionMode(CheckableListView.ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(CheckableListView.ActionMode mode, MenuItem item) {
      if (listing.processActionItemClick(item.getItemId())) {
        return true;
      } else {
        switch (item.getItemId()) {
          case R.id.action_delete:
            service.getPlaylistControl().delete(listing.getCheckedItemIds());
            break;
          default:
            return false;
        }
        mode.finish();
        return true;
      }
    }

    @Override
    public void onDestroyActionMode(CheckableListView.ActionMode mode) {
    }

    @Override
    public void onItemCheckedStateChanged(CheckableListView.ActionMode mode, int position, long id,
        boolean checked) {
      mode.setTitle(getActionModeTitle());
    }
  }
}
