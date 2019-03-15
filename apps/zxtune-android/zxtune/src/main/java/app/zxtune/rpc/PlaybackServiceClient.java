/**
 * @file
 * @brief Remote client stub of PlaybackService
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.rpc;

import android.net.Uri;
import android.os.DeadObjectException;
import android.os.RemoteException;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.PlaylistControl;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

import java.util.concurrent.TimeUnit;

public final class PlaybackServiceClient implements PlaybackService {

  private static final String TAG = PlaybackServiceClient.class.getName();

  private final IRemotePlaybackService delegate;
  private final PlaylistControl playlist;
  private final PlaybackControl playback;
  private final SeekControl seek;
  private final Visualizer visualizer;
  private final CompositeCallback callbacks;
  private final IRemoteCallback callbackDelegate;

  public PlaybackServiceClient(IRemotePlaybackService delegate) {
    this.delegate = delegate;
    this.playlist = new PlaylistControlClient();
    this.playback = new PlaybackControlClient();
    this.seek = new SeekControlClient();
    this.visualizer = new VisualizerClient();
    this.callbacks = new CompositeCallback();
    this.callbackDelegate = new CallbackServer(callbacks);
    try {
      delegate.subscribe(callbackDelegate);
    } catch (RemoteException e) {
      Log.w(TAG, e, "subscribe()");
    }
  }

  @Override
  public void setNowPlaying(Uri[] uris) {
    try {
      delegate.setNowPlaying(uris);
    } catch (RemoteException e) {
      Log.w(TAG, e, "setNowPlaying()");
    }
  }

  @Override
  public PlaylistControl getPlaylistControl() {
    return playlist;
  }

  @Override
  public PlaybackControl getPlaybackControl() {
    return playback;
  }

  @Override
  public SeekControl getSeekControl() {
    return seek;
  }

  @Override
  public Visualizer getVisualizer() {
    return visualizer;
  }

  @Override
  public void subscribe(Callback callback) {
    callbacks.add(callback);
  }

  @Override
  public void unsubscribe(Callback callback) {
    callbacks.remove(callback);
    if (callbacks.isEmpty()) {
      try {
        delegate.unsubscribe(callbackDelegate);
      } catch (RemoteException e) {
        Log.w(TAG, e, "unsubscribe()");
        callbacks.add(callback);
      }
    }
  }

  private class PlaylistControlClient implements PlaylistControl {

    @Override
    public void add(Uri[] uris) {
      try {
        delegate.add(uris);
      } catch (RemoteException e) {
        Log.w(TAG, e, "add()");
      }
    }

    @Override
    public void delete(long[] ids) {
      try {
        delegate.delete(ids);
      } catch (RemoteException e) {
        Log.w(TAG, e, "delete()");
      }
    }

    @Override
    public void deleteAll() {
      try {
        delegate.deleteAll();
      } catch (RemoteException e) {
        Log.w(TAG, e, "deleteAll()");
      }
    }

    @Override
    public void move(long id, int delta) {
      try {
        delegate.move(id, delta);
      } catch (RemoteException e) {
        Log.w(TAG, e, "move()");
      }
    }

    @Override
    public void sort(SortBy field, SortOrder order) {
      try {
        delegate.sort(field.name(), order.name());
      } catch (RemoteException e) {
        Log.w(TAG, e, "sort()");
      }
    }
  }

  private class PlaybackControlClient implements PlaybackControl {

    @Override
    public void play() {
      try {
        delegate.play();
      } catch (RemoteException e) {
        Log.w(TAG, e, "play()");
      }
    }

    @Override
    public void stop() {
      try {
        delegate.stop();
      } catch (RemoteException e) {
        Log.w(TAG, e, "stop()");
      }
    }

    @Override
    public void togglePlayStop() {
      try {
        delegate.togglePlayStop();
      } catch (RemoteException e) {
        Log.w(TAG, e, "togglePlayStop()");
      }
    }

    @Override
    public void next() {
      try {
        delegate.next();
      } catch (RemoteException e) {
        Log.w(TAG, e, "next()");
      }
    }

    @Override
    public void prev() {
      try {
        delegate.prev();
      } catch (RemoteException e) {
        Log.w(TAG, e, "prev()");
      }
    }

    @Override
    public TrackMode getTrackMode() {
      try {
        return TrackMode.values()[delegate.getTrackMode()];
      } catch (RemoteException e) {
        Log.w(TAG, e, "getTrackMode()");
        return TrackMode.REGULAR;
      }
    }

    @Override
    public void setTrackMode(TrackMode mode) {
      try {
        delegate.setTrackMode(mode.ordinal());
      } catch (RemoteException e) {
        Log.w(TAG, e, "setTrackMode()");
      }
    }

    @Override
    public SequenceMode getSequenceMode() {
      try {
        return SequenceMode.values()[delegate.getSequenceMode()];
      } catch (RemoteException e) {
        Log.w(TAG, e, "getSequenceMode()");
        return SequenceMode.ORDERED;
      }
    }

    @Override
    public void setSequenceMode(SequenceMode mode) {
      try {
        delegate.setSequenceMode(mode.ordinal());
      } catch (RemoteException e) {
        Log.w(TAG, e, "setSequenceMode()");
      }
    }
  }

  private class SeekControlClient implements SeekControl {

    @Override
    public TimeStamp getDuration() {
      try {
        return TimeStamp.createFrom(delegate.getDuration(), TimeUnit.MILLISECONDS);
      } catch (RemoteException e) {
        Log.w(TAG, e, "getDuration()");
        return TimeStamp.EMPTY;
      }
    }

    @Override
    public TimeStamp getPosition() {
      try {
        return TimeStamp.createFrom(delegate.getPosition(), TimeUnit.MILLISECONDS);
      } catch (RemoteException e) {
        Log.w(TAG, e, "getPosition()");
        return TimeStamp.EMPTY;
      }
    }

    @Override
    public void setPosition(TimeStamp pos) {
      try {
        delegate.setPosition(pos.convertTo(TimeUnit.MILLISECONDS));
      } catch (RemoteException e) {
        Log.w(TAG, e, "setPosition()");
      }
    }
  }

  private class VisualizerClient implements Visualizer {

    @Override
    public int getSpectrum(int[] bands, int[] levels) {
      try {
        final int[] packed = delegate.getSpectrum();
        for (int i = 0; i != packed.length; ++i) {
          bands[i] = packed[i] & 0xff;
          levels[i] = packed[i] >> 8;
        }
        return packed.length;
      } catch (DeadObjectException e) {
        throw new IllegalStateException(e);
      } catch (RemoteException e) {
        Log.w(TAG, e, "getSpectrum()");
        return 0;
      }
    }
  }

  private static class CallbackServer extends IRemoteCallback.Stub {

    private final Callback delegate;

    CallbackServer(Callback delegate) {
      this.delegate = delegate;
    }

    @Override
    public void onInitialState(int state) {
      delegate.onInitialState(PlaybackControl.State.values()[state]);
    }

    @Override
    public void onStateChanged(int state, long pos) {
      delegate.onStateChanged(PlaybackControl.State.values()[state], TimeStamp.createFrom(pos, TimeUnit.MILLISECONDS));
    }

    @Override
    public void onItemChanged(ParcelablePlaybackItem item) {
      delegate.onItemChanged(item);
    }

    @Override
    public void onError(String error) {
      delegate.onError(error);
    }
  }
}
