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

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
  private final NowPlayingState playingState;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }
  
  public PlaylistFragment() {
    this.playingState = new NowPlayingState();
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
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnItemClickListener(new OnItemClickListener());
    listing.setOnItemLongClickListener(new OnItemLongClickListener());
    listing.setPlayitemStateSource(playingState);
    listing.setEmptyView(view.findViewById(R.id.playlist_stub));
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
      final Uri[] toPlay = {Query.unparse(id)};
      service.setNowPlaying(toPlay);
    }
  }
  
  private class OnItemLongClickListener implements PlaylistView.OnItemLongClickListener {

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int pos, long id) {
      final long[] toDelete = {id};
      service.getPlaylistControl().delete(toDelete);
      return true;
    }
  }
  
  private class NowPlayingState implements Callback, PlaylistView.PlayitemStateSource {
    
    private final Runnable updateTask;
    private boolean isPlaying;
    private Uri nowPlaying;
    
    public NowPlayingState() {
      this.updateTask = new Runnable() {
        @Override
        public void run() {
          listing.invalidateViews();
        }
      };
      nowPlaying = Uri.EMPTY;
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

    @Override
    public boolean isPlaying(Uri playlistUri) {
      return isPlaying && 0 == playlistUri.compareTo(nowPlaying);
    }
    
    private void updateView() {
      listing.removeCallbacks(updateTask);
      listing.postDelayed(updateTask, 100);
    }
  }
}
