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
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.RetainedCallbackSubscriptionFragment;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;
import app.zxtune.playlist.Query;

public class PlaylistFragment extends Fragment {

  private static final String TAG = PlaylistFragment.class.getName();
  private Releaseable connection;
  private Control control;
  private PlaylistView listing;
  private Uri nowPlaying = Uri.EMPTY;

  public static Fragment createInstance() {
    return new PlaylistFragment();
  }
  
  @Override
  public void onActivityCreated(Bundle savedInstanceState) {
    super.onActivityCreated(savedInstanceState);
    
    assert connection == null;

    Log.d(TAG, "Subscribe for service events");
    final CallbackSubscription subscription = RetainedCallbackSubscriptionFragment.find(getFragmentManager());
    connection = subscription.subscribe(new PlaybackCallback());
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.playlist, container, false) : null;
  }
  
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    final Activity activity = getActivity();
    final Cursor cursor = activity.getContentResolver().query(Query.unparse(null), null, null, null, null);
    activity.startManagingCursor(cursor);
    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnPlayitemClickListener(new ItemClickListener());
    listing.setPlayitemStateSource(new PlaylistView.PlayitemStateSource() {
      @Override
      public boolean isPlaying(Uri playlistUri) {
        return 0 == playlistUri.compareTo(nowPlaying);
      }
    });
    listing.setData(cursor);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();

    try {
      if (connection != null) {
        Log.d(TAG, "Unsubscribe from service events");
        connection.release();
      }
    } finally {
      connection = null;
    }
  }
  
  private class ItemClickListener implements PlaylistView.OnPlayitemClickListener {
    
    @Override
    public void onPlayitemClick(Uri playlistUri) {
      control.play(playlistUri);
    }
  
    @Override
    public boolean onPlayitemLongClick(Uri playlistUri) {
      getActivity().getContentResolver().delete(playlistUri, null, null);
      return true;
    }
  }

  private class PlaybackCallback implements Callback {
    
    @Override
    public void onControlChanged(Control control) {
      PlaylistFragment.this.control = control;
      final boolean connected = control != null;
      listing.setEnabled(connected);
      if (connected) {
        onItemChanged(control.getItem());
      } else {
        onStatusChanged(null);
      }
    }
  
    @Override
    public void onStatusChanged(Status status) {
      listing.invalidateViews();
      if (status.equals(Status.STOPPED)) {
        nowPlaying = Uri.EMPTY;
      }
    }
    
    @Override
    public void onItemChanged(Item item) {
      nowPlaying = item != null ? item.getId() : Uri.EMPTY;
    }
  }
}
