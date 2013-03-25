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
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import app.zxtune.Playback;
import app.zxtune.PlaybackService;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.playlist.Query;
import app.zxtune.ui.Position.StatusCallback;

public class Playlist extends Fragment implements PlaylistView.OnPlayitemClickListener {

  private Playback.Control control;
  private PlaylistView listing;

  public void setControl(Playback.Control control) {
    this.control = control;
    this.control.registerCallback(new Playback.Callback() {
      @Override
      public void stopped() {
        listing.invalidateViews();
      }
      
      @Override
      public void started(Uri playlistUri, String description, int duration) {
        listing.invalidateViews();
      }
      
      @Override
      public void positionChanged(int curFrame, String curTime) {
      }
      
      @Override
      public void paused(String description) {
      }
    });
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.playlist, null);
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
        final Uri nowPlaying = control.nowPlaying();
        final boolean res = 0 == playlistUri.compareTo(nowPlaying);
        Log.d(Playlist.class.getCanonicalName(), "isPlaying(" + playlistUri + ") for " + nowPlaying + " is " + res);
        return res;
      }
    });
    listing.setData(cursor);
  }

  @Override
  public void onPlayitemClick(Uri playlistUri) {
    final Context context = getActivity();
    final Intent intent = new Intent(Intent.ACTION_VIEW, playlistUri, context, PlaybackService.class);
    context.startService(intent);
  }

  @Override
  public boolean onPlayitemLongClick(Uri playlistUri) {
    getActivity().getContentResolver().delete(playlistUri, null, null);
    return true;
  }
}
