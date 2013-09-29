/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import app.zxtune.Releaseable;
import app.zxtune.ScanService;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.Query;
import app.zxtune.sound.AsyncPlayer;
import app.zxtune.sound.Player;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.StubPlayer;

public class PlaybackServiceLocal implements PlaybackService, Releaseable {
  
  private static final String TAG = PlaybackServiceLocal.class.getName();
  
  private final Context context;
  private final CompositeCallback callbacks;
  private final PlaylistControl playlist;
  private final PlaybackControl playback;
  private final SeekControl seek;
  private final Visualizer visualizer;
  private Holder holder;

  public PlaybackServiceLocal(Context context) {
    this.context = context;
    this.callbacks = new CompositeCallback();
    this.playlist = new DispatchedPlaylistControl();
    this.playback = new DispatchedPlaybackControl();
    this.seek = new DispatchedSeekControl();
    this.visualizer = new DispatchedVisualizer();
    this.holder = new Holder();
  }

  @Override
  public synchronized Item getNowPlaying() {
    return holder.item;
  }
  
  @Override
  public void setNowPlaying(Uri[] uris) {
    try {
      final Iterator iter = Iterator.create(context, uris);
      play(iter);
    } catch (Error e) {
      Log.w(TAG, "setNowPlaying()", e);
    }
  }
  
  private void play(Iterator iter) {
    final PlayerEventsListener events = new PlaybackEvents(callbacks, new DispatchedPlaybackControl());
    setNewHolder(new Holder(iter, events));
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
  }

  @Override
  public void release() {
    synchronized (this) {
      try {
        holder.release();
      } finally {
        holder = new Holder();
      }
    }
  }
  
  private void setNewHolder(Holder holder) {
    final Holder oldHolder = this.holder;
    oldHolder.player.stopPlayback();
    synchronized (this) {
      this.holder = holder;
    }
    try {
      callbacks.onItemChanged(holder.item);
      holder.player.startPlayback();
    } finally {
      oldHolder.release();
    }
  }

  private static class Holder implements Releaseable {

    public final Iterator iterator;
    public final PlayableItem item;
    public final Player player;
    public final SeekControl seek;
    public final Visualizer visualizer;
    
    Holder() {
      this.iterator = IteratorStub.instance();
      this.item = PlayableItemStub.instance();
      this.player = StubPlayer.instance();
      this.seek = SeekControlStub.instance();
      this.visualizer = VisualizerStub.instance();
    }
    
    Holder(Iterator iterator, PlayerEventsListener events) {
      this.iterator = iterator;
      this.item = iterator.getItem();
      final ZXTune.Player lowPlayer = item.getModule().createPlayer();
      final SeekableSamplesSource source = new SeekableSamplesSource(lowPlayer, item.getDuration());
      this.player = AsyncPlayer.create(source, events);
      this.seek = source;
      this.visualizer = new PlaybackVisualizer(lowPlayer);
    }
    
    @Override
    public void release() {
      player.release();
      item.release();
    }
  }
  
  private final class DispatchedPlaylistControl implements PlaylistControl {

    @Override
    public void add(Uri[] uris) {
      final Intent intent = new Intent(context, ScanService.class);
      intent.setAction(ScanService.ACTION_START);
      intent.putExtra(ScanService.EXTRA_PATHS, uris);
      context.startService(intent);
    }
    
    @Override
    public void delete(long[] ids) {
      //ids => '[a, b, c]'
      final String rawArgs = Arrays.toString(ids);
      final String args = rawArgs.substring(1, rawArgs.length() - 1);
      final Uri uri = Query.unparse(null);
      //placeholders doesn't work and has limitations
      final String where = Database.Tables.Playlist.Fields._id + " IN (" + args + ")"; 
      context.getContentResolver().delete(uri, where, null);
    }
    
    @Override
    public void deleteAll() {
      final Uri uri = Query.unparse(null);
      context.getContentResolver().delete(uri, null, null);
    }
  }
  
  private final class DispatchedPlaybackControl implements PlaybackControl {
    
    @Override
    public void play() {
      synchronized (PlaybackServiceLocal.this) {
        holder.player.startPlayback();
      }
    }

    @Override
    public synchronized void stop() {
      synchronized (PlaybackServiceLocal.this) {
        holder.player.stopPlayback();
      }
    }

    @Override
    public synchronized boolean isPlaying() {
      synchronized (PlaybackServiceLocal.this) {
        return holder.player.isStarted();
      }
    }

    @Override
    public synchronized void next() {
      synchronized (PlaybackServiceLocal.this) {
        if (holder.iterator.next()) {
          PlaybackServiceLocal.this.play(holder.iterator);
        }
      }
    }

    @Override
    public synchronized void prev() {
      synchronized (PlaybackServiceLocal.this) {
        if (holder.iterator.prev()) {
          PlaybackServiceLocal.this.play(holder.iterator);
        }
      }
    }
  }
  
  private final class DispatchedSeekControl implements SeekControl {

    @Override
    public synchronized TimeStamp getDuration() {
      synchronized (PlaybackServiceLocal.this) {
        return holder.seek.getDuration();
      }
    }

    @Override
    public synchronized TimeStamp getPosition() {
      synchronized (PlaybackServiceLocal.this) {
        return holder.seek.getPosition();
      }
    }

    @Override
    public synchronized void setPosition(TimeStamp position) {
      synchronized (PlaybackServiceLocal.this) {
        holder.seek.setPosition(position);
      }
    }
  }
  
  private final class DispatchedVisualizer implements Visualizer {
    
    @Override
    public synchronized int getSpectrum(int[] bands, int[] levels) {
      synchronized (PlaybackServiceLocal.this) {
        return holder.visualizer.getSpectrum(bands, levels);
      }
    }
  }

  private static final class PlaybackEvents implements PlayerEventsListener {

    private final Callback callback;
    private final PlaybackControl ctrl;
    
    public PlaybackEvents(Callback callback, PlaybackControl ctrl) {
      this.callback = callback;
      this.ctrl = ctrl;
    }
    
    @Override
    public void onStart() {
      callback.onStatusChanged(true);
    }

    @Override
    public void onFinish() {
      new Thread(new Runnable() {
        @Override
        public void run() {
          ctrl.next();
        }
      }).start();
    }

    @Override
    public void onStop() {
      callback.onStatusChanged(false);
    }

    @Override
    public void onError(Error e) {
      Log.d(TAG, "Error occurred: " + e);
    }
  }
  
  private static final class SeekableSamplesSource implements SamplesSource, SeekControl {

    private ZXTune.Player player;
    private final TimeStamp totalDuration;
    private final TimeStamp frameDuration;
    private volatile TimeStamp seekRequest;
    
    public SeekableSamplesSource(ZXTune.Player player, TimeStamp totalDuration) {
      this.player = player;
      this.totalDuration = totalDuration;
      final long frameDurationUs = player.getProperty(ZXTune.Properties.Sound.FRAMEDURATION, ZXTune.Properties.Sound.FRAMEDURATION_DEFAULT); 
      this.frameDuration = TimeStamp.createFrom(frameDurationUs, TimeUnit.MICROSECONDS);
      player.setPosition(0);
    }
    
    @Override
    public void initialize(int sampleRate) {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, sampleRate);
    }

    @Override
    public boolean getSamples(short[] buf) {
      if (seekRequest != null) {
        final int frame = (int) seekRequest.divides(frameDuration); 
        player.setPosition(frame);
        seekRequest = null;
      }
      return player.render(buf);
    }

    @Override
    public void release() {
      player.release();
      player = null;
    }
    
    @Override
    public TimeStamp getDuration() {
      return totalDuration; 
    }

    @Override
    public TimeStamp getPosition() {
      TimeStamp res = seekRequest;
      if (res == null) {
        final int frame = player.getPosition();
        res = frameDuration.multiplies(frame); 
      }
      return res;
    }

    @Override
    public void setPosition(TimeStamp pos) {
      seekRequest = pos;
    }
  }
  
  private static final class PlaybackVisualizer implements Visualizer {
    
    private final ZXTune.Player player;
    
    public PlaybackVisualizer(ZXTune.Player player) {
      this.player = player;
    }

    @Override
    public int getSpectrum(int[] bands, int[] levels) {
      return player.analyze(bands, levels);
    }
  }
}
