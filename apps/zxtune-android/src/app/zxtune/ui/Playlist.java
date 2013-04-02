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

import java.io.Closeable;
import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.Playback;
import app.zxtune.Playback.Item;
import app.zxtune.Playback.Status;
import app.zxtune.PlaybackService;
import app.zxtune.R;
import app.zxtune.playlist.Query;
import app.zxtune.rpc.BroadcastPlaybackCallbackReceiver;

public class Playlist extends Fragment implements PlaylistView.OnPlayitemClickListener, Playback.Callback {

  private Playback.Control control;
  private PlaylistView listing;
  private Closeable callbackHandler;
  private Uri nowPlaying = Uri.EMPTY;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.playlist, null);
  }
  
  public void setControl(Playback.Control control) {
    this.control = control;
    getView().setEnabled(control != null);
    if (control != null) {
      itemChanged(control.getItem());
    }
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    final Activity activity = getActivity();
    final Cursor cursor = activity.getContentResolver().query(Query.unparse(null), null, null, null, null);
    activity.startManagingCursor(cursor);
    listing = (PlaylistView) view.findViewById(R.id.playlist_content);
    listing.setOnPlayitemClickListener(this);
    listing.setPlayitemStateSource(new PlaylistView.PlayitemStateSource() {
      @Override
      public boolean isPlaying(Uri playlistUri) {
        return 0 == playlistUri.compareTo(nowPlaying);
      }
    });
    listing.setData(cursor);
  }

  @Override
  public void onStart() {
    super.onStart();
    callbackHandler = new BroadcastPlaybackCallbackReceiver(getActivity(), this);
  }

  @Override
  public void onStop() {
    super.onStop();
    try {
      callbackHandler.close();
    } catch (IOException e) {
    } finally {
      callbackHandler = null;
    }
  }
  
  @Override
  public void onPlayitemClick(Uri playlistUri) {
    control.play(playlistUri);
  }

  @Override
  public boolean onPlayitemLongClick(Uri playlistUri) {
    getActivity().getContentResolver().delete(playlistUri, null, null);
    return true;
  }

  @Override
  public void statusChanged(Status status) {
    //listing.invalidateViews();
    if (status == null) {
      nowPlaying = Uri.EMPTY;
    }
  }
  
  @Override
  public void itemChanged(Item item) {
    nowPlaying = item != null ? item.getId() : Uri.EMPTY;
  }
}
