/*
 * @file
 * @brief Remote stub for Playback.Control
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.util.concurrent.TimeUnit;

import android.net.Uri;
import android.os.RemoteException;
import android.util.Log;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

public final class PlaybackServiceClient implements PlaybackService {

  private static final String TAG = PlaybackServiceClient.class.getName();
  
  private final IRemotePlaybackService delegate;
  private final PlaybackControl playback;
  private final SeekControl seek;
  private final Visualizer visualizer;
  private final CompositeCallback callbacks;
  private final IRemoteCallback callbackDelegate;

  public PlaybackServiceClient(IRemotePlaybackService delegate) {
    this.delegate = delegate;
    this.playback = new PlaybackControlClient();
    this.seek = new SeekControlClient();
    this.visualizer = new VisualizerClient();
    this.callbacks = new CompositeCallback();
    this.callbackDelegate = new CallbackServer(callbacks);
  }
  
  @Override
  public Item getNowPlaying() {
    try {
      return delegate.getNowPlaying();
    } catch (RemoteException e) {
      Log.e(TAG, "getNowPlaying()", e);
      return ItemStub.instance();
    }
  }
  
  @Override
  public void setNowPlaying(Uri uri) {
    try {
      delegate.setNowPlaying(uri);
    } catch (RemoteException e) {
      Log.e(TAG, "setNowPlaying()", e);
    }
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
    if (1 == callbacks.add(callback)) {
      try {
        delegate.subscribe(callbackDelegate);
      } catch (RemoteException e) {
        Log.e(TAG, "subscribe()", e);
        callbacks.remove(callback);
      }
    }
  }
  
  @Override
  public void unsubscribe(Callback callback) {
    if (0 == callbacks.remove(callback)) {
      try {
        delegate.unsubscribe(callbackDelegate);
      } catch (RemoteException e) {
        Log.e(TAG, "unsubscribe()", e);
        callbacks.add(callback);
      }
    }
  }
  
  private class PlaybackControlClient implements PlaybackControl  {
    
    @Override
    public void play() {
      try {
        delegate.play();
      } catch (RemoteException e) {
        Log.e(TAG, "play()", e);
      }
    }
    
    @Override
    public void stop() {
      try {
        delegate.stop();
      } catch (RemoteException e) {
        Log.e(TAG, "stop()", e);
      }
    }
    
    @Override
    public boolean isPlaying() {
      try {
        return delegate.isPlaying();
      } catch (RemoteException e) {
        Log.e(TAG, "isPlaying()", e);
        return false;
      }
    }
    
    @Override
    public void next() {
      try {
        delegate.next();
      } catch (RemoteException e) {
        Log.e(TAG, "next()", e);
      }
    }
    
    @Override
    public void prev() {
      try {
        delegate.prev();
      } catch (RemoteException e) {
        Log.e(TAG, "prev()", e);
      }
    }
  }
  
  private class SeekControlClient implements SeekControl {
  
    @Override
    public TimeStamp getDuration() {
      try {
        return TimeStamp.createFrom(delegate.getDuration(), TimeUnit.MILLISECONDS);
      } catch (RemoteException e) {
        Log.e(TAG, "getDuration()", e);
        return TimeStamp.EMPTY;
      }
    }

    @Override
    public TimeStamp getPosition() {
      try {
        return TimeStamp.createFrom(delegate.getPosition(), TimeUnit.MILLISECONDS);
      } catch (RemoteException e) {
        Log.e(TAG, "getPosition()", e);
        return TimeStamp.EMPTY;
      }
    }

    @Override
    public void setPosition(TimeStamp pos) {
      try {
        delegate.setPosition(pos.convertTo(TimeUnit.MILLISECONDS));
      } catch (RemoteException e) {
        Log.e(TAG, "setPosition()", e);
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
      } catch (RemoteException e) {
        Log.e(TAG, "getSpectrum()", e);
        return 0;
      }
    }
  }
  
  private static class CallbackServer extends IRemoteCallback.Stub {
    
    private final Callback delegate;
    
    public CallbackServer(Callback delegate) {
      this.delegate = delegate;
    }
    
    @Override
    public void onStatusChanged(boolean isPlaying) {
      delegate.onStatusChanged(isPlaying);
    }
    
    @Override
    public void onItemChanged(ParcelablePlaybackItem item) {
      delegate.onItemChanged(item);
    }
  }
}
