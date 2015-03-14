/**
 *
 * @file
 *
 * @brief Local implementation of PlaybackService
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import java.io.IOException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.util.Log;
import app.zxtune.Preferences;
import app.zxtune.Releaseable;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.sound.AsyncPlayer;
import app.zxtune.sound.Player;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.SamplesTarget;
import app.zxtune.sound.SoundOutputSamplesTarget;
import app.zxtune.sound.StubPlayer;

public class PlaybackServiceLocal implements PlaybackService, Releaseable {
  
  private static final String TAG = PlaybackServiceLocal.class.getName();
  
  private final static String PREF_LAST_PLAYED_PATH = "last_played_path";
  private final static String PREF_LAST_PLAYED_POSITION = "last_played_position";
  
  private final Context context;
  private final ExecutorService executor;
  private final CompositeCallback callbacks;
  private final PlaylistControl playlist;
  private final PlaybackControl playback;
  private final SeekControl seek;
  private final Visualizer visualizer;
  private Holder holder;

  private static interface Command {
    void execute() throws IOException;
  }
    
  public PlaybackServiceLocal(Context context) {
    this.context = context;
    this.executor = Executors.newSingleThreadExecutor();
    this.callbacks = new CompositeCallback();
    this.playlist = new PlaylistControlLocal(context);
    this.playback = new DispatchedPlaybackControl();
    this.seek = new DispatchedSeekControl();
    this.visualizer = new DispatchedVisualizer();
    this.holder = new Holder();
  }

  @Override
  public synchronized Item getNowPlaying() {
    return holder.item;
  }
  
  public final void restoreSession() {
    final SharedPreferences prefs = Preferences.getDefaultSharedPreferences(context);
    final String path = prefs.getString(PREF_LAST_PLAYED_PATH, null);
    if (path != null) {
      final long position = prefs.getLong(PREF_LAST_PLAYED_POSITION, 0);
      Log.d(TAG, String.format("Restore last played item '%s' at %dms", path, position));
      executeCommand(new RestoreSessionCommand(Uri.parse(path), TimeStamp.createFrom(position, TimeUnit.MILLISECONDS)));
    }
  }
  
  private class RestoreSessionCommand implements Command {

    private final Uri[] uris;
    private final TimeStamp position;
    
    RestoreSessionCommand(Uri uri, TimeStamp position) {
      this.uris = new Uri[] {uri};
      this.position = position;
    }
    
    @Override
    public void execute() throws IOException {
      final Iterator iter = IteratorFactory.createIterator(context, uris);
      setNewIterator(iter);
      seek.setPosition(position);
    }
  }
  
  public final void storeSession() {
    executeCommand(new StoreSessionCommand());
  }
  
  private class StoreSessionCommand implements Command {
    @Override
    public void execute() throws IOException {
      final Uri nowPlaying = getNowPlaying().getId();
      if (!nowPlaying.equals(Uri.EMPTY)) {
        final String path = nowPlaying.toString();
        final long position = getSeekControl().getPosition().convertTo(TimeUnit.MILLISECONDS);
        Log.d(TAG, String.format("Save last played item '%s' at %dms", path, position));
        final SharedPreferences.Editor editor = Preferences.getDefaultSharedPreferences(context).edit();
        editor.putString(PREF_LAST_PLAYED_PATH, path);
        editor.putLong(PREF_LAST_PLAYED_POSITION, position);
        editor.apply();
      }
    }
  }
  
  @Override
  public void setNowPlaying(Uri[] uris) {
    executeCommand(new SetNowPlayingCommand(uris));
  }
  
  private class SetNowPlayingCommand implements Command {
    
    private final Uri[] uris;
    
    SetNowPlayingCommand(Uri[] uris) {
      this.uris = uris;
    }

    @Override
    public void execute() throws IOException {
      final Iterator iter = IteratorFactory.createIterator(context, uris);
      play(iter);
    }
  }
  
  private void setNewIterator(Iterator iter) throws IOException {
    final PlayerEventsListener events = new PlaybackEvents(callbacks, playback, seek);
    setNewHolder(new Holder(iter, events));
  }
  
  private void play(Iterator iter) throws IOException {
    setNewIterator(iter);
    holder.player.startPlayback();
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
    shutdownExecutor();
    synchronized (this) {
      try {
        holder.player.stopPlayback();
        holder.release();
      } finally {
        holder = new Holder();
      }
    }
  }
  
  private void shutdownExecutor() {
    try {
      do {
        executor.shutdownNow();
        Log.d(TAG, "Waiting for executor shutdown...");
      } while (!executor.awaitTermination(10, TimeUnit.SECONDS));
      Log.d(TAG, "Executor shut down");
    } catch (InterruptedException e) {
      Log.w(TAG, "Failed to shutdown executor", e);
    }
  }
  
  private void executeCommand(final Command cmd) {
    executor.execute(new Runnable() {

      @Override
      public void run() {
        try {
          callbacks.onIOStatusChanged(true);
          cmd.execute();
        } catch (Exception e) {//IOException|InterruptedException
          Log.w(TAG, cmd.getClass().getName(), e);
          final Throwable cause = e.getCause();
          final String msg = cause != null ? cause.getMessage() : e.getMessage();
          callbacks.onError(msg);
        } finally {
          callbacks.onIOStatusChanged(false);
        }
      }
    });
  }
  
  private void setNewHolder(Holder holder) {
    final Holder oldHolder = this.holder;
    oldHolder.player.stopPlayback();
    synchronized (this) {
      this.holder = holder;
    }
    try {
      callbacks.onItemChanged(holder.item);
    } finally {
      oldHolder.release();
    }
  }
  
  private void saveProperty(String name, long value) {
    final SharedPreferences prefs = Preferences.getDefaultSharedPreferences(context);
    prefs.edit().putLong(name, value).apply();
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
    
    Holder(Iterator iterator, PlayerEventsListener events) throws IOException {
      this.iterator = iterator;
      this.item = iterator.getItem();
      final ZXTune.Player lowPlayer = item.getModule().createPlayer();
      final SeekableSamplesSource source = new SeekableSamplesSource(lowPlayer, item.getDuration());
      final SamplesTarget target = SoundOutputSamplesTarget.create();
      this.player = AsyncPlayer.create(source, target, events);
      this.seek = source;
      this.visualizer = new PlaybackVisualizer(lowPlayer);
    }
    
    @Override
    public void release() {
      player.release();
      item.release();
    }
  }
  
  private final class DispatchedPlaybackControl implements PlaybackControl {
    
    private final IteratorFactory.NavigationMode navigation;
    
    DispatchedPlaybackControl() {
      this.navigation = new IteratorFactory.NavigationMode(context);
    }
    
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
        executeCommand(new PlayNextCommand(holder.iterator));
      }
    }

    @Override
    public synchronized void prev() {
      synchronized (PlaybackServiceLocal.this) {
        executeCommand(new PlayPrevCommand(holder.iterator));
      }
    }

    @Override
    public TrackMode getTrackMode() {
      final long val = ZXTune.GlobalOptions.instance().getProperty(ZXTune.Properties.Sound.LOOPED, 0);
      return val != 0 ? TrackMode.LOOPED : TrackMode.REGULAR;
    }
    
    @Override
    public void setTrackMode(TrackMode mode) {
      final long val = mode == TrackMode.LOOPED ? 1 : 0;
      ZXTune.GlobalOptions.instance().setProperty(ZXTune.Properties.Sound.LOOPED, val);
      saveProperty(ZXTune.Properties.Sound.LOOPED, val);
    }
    
    @Override
    public SequenceMode getSequenceMode() {
      return navigation.get();
    }
    
    @Override
    public void setSequenceMode(SequenceMode mode) {
      navigation.set(mode);
    }
  }
  
  private class PlayNextCommand implements Command {
    
    private final Iterator iter;
    
    PlayNextCommand(Iterator iter) {
      this.iter = iter;
    }

    @Override
    public void execute() throws IOException {
      if (iter.next()) {
        play(iter);
      }
    }
  }

  private class PlayPrevCommand implements Command {
    
    private final Iterator iter;
    
    PlayPrevCommand(Iterator iter) {
      this.iter = iter;
    }

    @Override
    public void execute() throws IOException {
      if (iter.prev()) {
        play(iter);
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
    private final SeekControl seek;
    
    public PlaybackEvents(Callback callback, PlaybackControl ctrl, SeekControl seek) {
      this.callback = callback;
      this.ctrl = ctrl;
      this.seek = seek;
    }
    
    @Override
    public void onStart() {
      callback.onStatusChanged(true);
    }

    @Override
    public void onFinish() {
      seek.setPosition(TimeStamp.EMPTY);
      ctrl.next();
    }

    @Override
    public void onStop() {
      callback.onStatusChanged(false);
    }

    @Override
    public void onError(Exception e) {
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
