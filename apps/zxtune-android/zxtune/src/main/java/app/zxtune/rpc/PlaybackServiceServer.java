/**
 *
 * @file
 *
 * @brief Remote service stub for PlaybackService
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.rpc;

import android.net.Uri;
import android.os.RemoteException;
import android.support.annotation.Nullable;

import java.util.HashMap;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.playback.PlaybackControl.TrackMode;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.PlaylistControl;
import app.zxtune.playback.PlaylistControl.SortBy;
import app.zxtune.playback.PlaylistControl.SortOrder;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

public class PlaybackServiceServer extends IRemotePlaybackService.Stub {

  private static final String TAG = PlaybackServiceServer.class.getName();

  private final PlaybackService delegate;
  private final PlaylistControl playlist;
  private final PlaybackControl playback;
  private final SeekControl seek;
  private final Visualizer visualizer;
  private final HashMap<IRemoteCallback, Callback> callbacks;

  public PlaybackServiceServer(PlaybackService delegate) {
    this.delegate = delegate;
    this.playlist = delegate.getPlaylistControl();
    this.playback = delegate.getPlaybackControl();
    this.seek = delegate.getSeekControl();
    this.visualizer = delegate.getVisualizer();
    this.callbacks = new HashMap<>();
  }

  @Nullable
  @Override
  public ParcelablePlaybackItem getNowPlaying() {
    try {
      return ParcelablePlaybackItem.create(delegate.getNowPlaying());
    } catch (Exception e) {
      Log.w(TAG, e, "getNowPlaying()");
    }
    return null;
  }
  
  @Override
  public void setNowPlaying(Uri[] uris) {
    delegate.setNowPlaying(uris);
  }
  
  @Override
  public void add(Uri[] uris) {
    playlist.add(uris);
  }
  
  @Override
  public void delete(long[] ids) {
    playlist.delete(ids);
  }
  
  @Override
  public void deleteAll() {
    playlist.deleteAll();
  }
  
  @Override
  public void move(long id, int delta) {
    playlist.move(id, delta);
  }
  
  @Override
  public void sort(String field, String order) {
    playlist.sort(SortBy.valueOf(field), SortOrder.valueOf(order));
  }

  @Override
  public void play() {
    playback.play();
  }

  @Override
  public void stop() {
    playback.stop();
  }
  
  @Override
  public int getState() {
    return playback.getState().ordinal();
  }
    
  @Override
  public void next() {
    playback.next();
  }
  
  @Override
  public void prev() {
    playback.prev();
  }
  
  @Override
  public int getTrackMode() {
    return playback.getTrackMode().ordinal();
  }
  
  @Override
  public void setTrackMode(int mode) {
    playback.setTrackMode(TrackMode.values()[mode]);
  }
  
  @Override
  public int getSequenceMode() {
    return playback.getSequenceMode().ordinal();
  }
  
  @Override
  public void setSequenceMode(int mode) {
    playback.setSequenceMode(SequenceMode.values()[mode]);
  }
  
  @Override
  public long getDuration() {
    try {
      return seek.getDuration().convertTo(TimeUnit.MILLISECONDS);
    } catch (Exception e) {
      Log.w(TAG, e, "getDuration()");
    }
    return 0;
  }
  
  @Override
  public long getPosition() {
    try {
      return seek.getPosition().convertTo(TimeUnit.MILLISECONDS);
    } catch (Exception e) {
      Log.w(TAG, e, "getPosition()");
    }
    return 0;
  }
  
  @Override
  public void setPosition(long ms) {
    try {
      seek.setPosition(TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS));
    } catch (Exception e) {
      Log.w(TAG, e, "setPosition()");
    }
  }
  
  @Override
  public int[] getSpectrum() {
    final int MAX_BANDS = 96;
    final int[] bands = new int[MAX_BANDS];
    final int[] levels = new int[MAX_BANDS];
    try {
      final int chans = visualizer.getSpectrum(bands, levels);
      final int[] res = new int[chans];
      for (int idx = 0; idx != chans; ++idx) {
        res[idx] = (levels[idx] << 8) | bands[idx];
      }
      return res;
    } catch (Exception e) {
      Log.w(TAG, e, "getSpectrum()");
    }
    return new int[1];
  }
  
  @Override
  public void subscribe(IRemoteCallback callback) {
    final Callback client = new CallbackClient(callback);
    callbacks.put(callback, client);
    delegate.subscribe(client);
  }
  
  @Override
  public void unsubscribe(IRemoteCallback callback) {
    final Callback client = callbacks.get(callback);
    if (client != null) {
      delegate.unsubscribe(client);
      callbacks.remove(callback);
    }
  }
  
  private final class CallbackClient implements Callback {
    
    private final String TAG = CallbackClient.class.getName();
    private final IRemoteCallback delegate;
    
    public CallbackClient(IRemoteCallback delegate) {
      this.delegate = delegate;
    }

    @Override
    public void onInitialState(PlaybackControl.State state, Item item, boolean ioStatus) {
      try {
        delegate.onInitialState(state.ordinal(), ParcelablePlaybackItem.create(item), ioStatus);
      } catch (Exception e) {
        Log.w(TAG, e, "onInitialState()");
      }
    }

    @Override
    public void onStateChanged(PlaybackControl.State state) {
      try {
        delegate.onStateChanged(state.ordinal());
      } catch (RemoteException e) {
        Log.w(TAG, e, "onStatusChanged()");
      }
    }
    
    @Override
    public void onItemChanged(Item item) {
      try {
        delegate.onItemChanged(ParcelablePlaybackItem.create(item));
      } catch (Exception e) {
        Log.w(TAG, e, "onItemChanged()");
      }
    }
    
    @Override
    public void onIOStatusChanged(boolean isActive) {
      try {
        delegate.onIOStatusChanged(isActive);
      } catch (RemoteException e) {
        Log.w(TAG, e, "onIOStatusChanged()");
      }
    }
    
    @Override
    public void onError(String error) {
      try {
        delegate.onError(error);
      } catch (RemoteException e) {
        Log.w(TAG, e, "onError()");
      }
    }
  }
}
