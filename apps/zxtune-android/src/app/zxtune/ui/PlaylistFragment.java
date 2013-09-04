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
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.PlaylistControl;
import app.zxtune.playlist.Query;

public class PlaylistFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = PlaylistFragment.class.getName();
  private PlaybackService service;
  private Releaseable connection;
  private PlaylistState state;
  private PlaylistView listing;
  private volatile Uri nowPlaying = Uri.EMPTY;
  private volatile boolean isPlaying = false;

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
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    final Activity activity = getActivity();
    final Cursor cursor = activity.getContentResolver().query(Query.unparse(null), null, null, null, null);
    activity.startManagingCursor(cursor);
    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnPlayitemClickListener(new ItemClickListener());
    listing.setPlayitemStateSource(new PlaylistView.PlayitemStateSource() {
      @Override
      public boolean isPlaying(Uri playlistUri) {
        return isPlaying && 0 == playlistUri.compareTo(nowPlaying);
      }
    });
    listing.setData(cursor);
    listing.setEmptyView(view.findViewById(R.id.playlist_stub));
    
    bindViewToConnectedService();
  }

  @Override
  public void onStart() {
    super.onStart();
    listing.setSelection(state.getCurrentViewPosition());
  }
  
  @Override
  public void onStop() {
    super.onStop();
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }
  
  @Override
  public synchronized void onDestroy() {
    super.onDestroy();

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
  
  private class ItemClickListener implements PlaylistView.OnPlayitemClickListener {
    
    @Override
    public void onPlayitemClick(Uri playlistUri) {
      service.setNowPlaying(playlistUri);
    }
  
    @Override
    public boolean onPlayitemLongClick(Uri playlistUri) {
      service.getPlaylistControl().delete(playlistUri);
      return true;
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
