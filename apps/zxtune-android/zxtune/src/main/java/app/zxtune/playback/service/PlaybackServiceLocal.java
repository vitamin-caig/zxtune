/**
 * @file
 * @brief Local implementation of PlaybackService
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback.service;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.Releaseable;
import app.zxtune.TimeStamp;
import app.zxtune.analytics.Analytics;
import app.zxtune.core.Properties;
import app.zxtune.device.sound.SoundOutputSamplesTarget;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.Item;
import app.zxtune.playback.Iterator;
import app.zxtune.playback.IteratorFactory;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.stubs.IteratorStub;
import app.zxtune.playback.stubs.PlayableItemStub;
import app.zxtune.playback.stubs.VisualizerStub;
import app.zxtune.preferences.DataStore;
import app.zxtune.sound.AsyncPlayer;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.SamplesTarget;
import app.zxtune.sound.StubSamplesSource;

public class PlaybackServiceLocal implements PlaybackService, Releaseable {

  private static final String TAG = PlaybackServiceLocal.class.getName();

  private static final String PREF_LAST_PLAYED_PATH = "last_played_path";
  private static final String PREF_LAST_PLAYED_POSITION = "last_played_position";

  private final Context context;
  private final DataStore prefs;
  private final ExecutorService executor;
  private final CompositeCallback callbacks;
  private final NavigateCommand navigateCmd;
  private final ActivateCommand activateCmd;
  private final DispatchedPlaybackControl playback;
  private final DispatchedSeekControl seek;
  private final DispatchedVisualizer visualizer;
  private final AtomicReference<Iterator> iterator;
  private final AtomicReference<Holder> holder;
  private final AsyncPlayer player;

  private interface Command {
    void execute() throws Exception;
  }

  public PlaybackServiceLocal(Context context, DataStore prefs) {
    this.context = context;
    this.prefs = prefs;
    this.executor = Executors.newCachedThreadPool();
    this.callbacks = new CompositeCallback();
    this.navigateCmd = new NavigateCommand();
    this.activateCmd = new ActivateCommand();
    this.playback = new DispatchedPlaybackControl();
    this.seek = new DispatchedSeekControl();
    this.visualizer = new DispatchedVisualizer();
    final SamplesTarget target = SoundOutputSamplesTarget.create(context);
    final PlayerEventsListener events = new PlaybackEvents(callbacks, playback, seek);
    this.iterator = new AtomicReference<>(IteratorStub.instance());
    this.holder = new AtomicReference<>(Holder.instance());
    this.player = AsyncPlayer.create(target, events);
    callbacks.onInitialState(PlaybackControl.State.STOPPED);
  }

  public final Item getNowPlaying() {
    return holder.get().item;
  }

  private PlaybackControl.State getState() {
    return player.isStarted() ? PlaybackControl.State.PLAYING : PlaybackControl.State.STOPPED;
  }

  private void storeSession() {
    try {
      final Uri nowPlaying = getNowPlaying().getId();
      if (!Uri.EMPTY.equals(nowPlaying)) {
        final String path = nowPlaying.toString();
        final long position = getSeekControl().getPosition().toMilliseconds();
        Log.d(TAG, "Save last played item '%s' at %dms", path, position);
        final Bundle bundle = new Bundle();
        bundle.putString(PREF_LAST_PLAYED_PATH, path);
        bundle.putLong(PREF_LAST_PLAYED_POSITION, position);
        prefs.putBatch(bundle);
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to store session");
    }
  }

  public final void restoreSession() {
    final String path = prefs.getString(PREF_LAST_PLAYED_PATH, null);
    if (path != null) {
      final long position = prefs.getLong(PREF_LAST_PLAYED_POSITION, 0);
      new Thread("RestoreSessionThread") {
        @Override
        public void run() {
          try {
            Log.d(TAG, "Restore last played item '%s' at %dms", path, position);
            restoreSession(Uri.parse(path), TimeStamp.fromMilliseconds(position));
          } catch (Exception e) {
            Log.w(TAG, e, "Failed to restore session");
          }
        }
      }.start();
    }
  }

  private void restoreSession(Uri uri, TimeStamp position) throws Exception {
    final Iterator iter = IteratorFactory.createIterator(context, uri, playback.navigation);
    final PlayableItem newItem = iter.getItem();
    final Holder newHolder = new Holder(newItem, player.getSampleRate());
    newHolder.source.setPosition(position);
    if (iterator.compareAndSet(IteratorStub.instance(), iter)) {
      setNewHolder(newHolder);
    } else {
      Log.d(TAG, "Drop stale session restore");
      newHolder.release();
    }
  }

  public final void setNowPlaying(Uri uri) {
    activateCmd.schedulePlay(uri);
  }

  private class ActivateCommand implements Command {

    private final AtomicReference<Uri> batch = new AtomicReference<>(null);

    final void schedulePlay(Uri uri) {
      if (null == batch.getAndSet(uri)) {
        executeCommand(this);
      }
    }

    @Override
    public void execute() throws Exception {
      for (; ; ) {
        final Uri uri = batch.get();
        try {
          final Iterator iter = IteratorFactory.createIterator(context, uri, playback.navigation);
          if (batch.compareAndSet(uri, null)) {
            iterator.set(iter);
            setNewItem(iter.getItem());
            player.startPlayback();
            break;
          }
        } catch (Exception e) {
          if (batch.compareAndSet(uri, null) || batch.compareAndSet(null, null)) {
            throw e;
          }
        }
      }
    }
  }

  private void setNewItem(PlayableItem newItem) {
    final Holder newHolder = new Holder(newItem, player.getSampleRate());
    setNewHolder(newHolder);
  }

  private void setNewHolder(Holder newHolder) {
    navigateCmd.cancel();
    player.setSource(newHolder.source);
    holder.getAndSet(newHolder).release();
    callbacks.onItemChanged(newHolder.item);
    callbacks.onStateChanged(getState(), TimeStamp.EMPTY);
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
  public Releaseable subscribe(Callback callback) {
    return callbacks.add(callback);
  }

  @Override
  public void release() {
    stopSync();
    shutdownExecutor();
    player.release();
    holder.getAndSet(Holder.instance()).release();
  }

  private void stopSync() {
    player.stopPlayback();
    storeSession();
  }

  private void shutdownExecutor() {
    try {
      for (int triesLeft = 2; triesLeft != 0; --triesLeft) {
        executor.shutdownNow();
        Log.d(TAG, "Waiting for executor shutdown...");
        if (executor.awaitTermination(1, TimeUnit.SECONDS)) {
          Log.d(TAG, "Executor shut down");
          return;
        }
      }
      throw new TimeoutException("No tries left");
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to shutdown executor");
    }
  }

  private void executeCommand(Command cmd) {
    try {
      executeCommandImpl(cmd);
    } catch (Exception e) {
      Log.w(TAG, e, cmd.getClass().getName());
    }
  }

  private void executeCommandImpl(final Command cmd) {
    executor.execute(() -> {
      try {
        cmd.execute();
      } catch (Exception e) {//IOException|InterruptedException
        Log.w(TAG, e, cmd.getClass().getName());
        final Throwable cause = e.getCause();
        final String msg = cause != null ? cause.getMessage() : e.getMessage();
        callbacks.onError(msg);
      }
    });
  }

  private static class Holder {

    final PlayableItem item;
    private final app.zxtune.core.Player player;
    final SamplesSource source;
    final Visualizer visualizer;

    private Holder() {
      this.item = PlayableItemStub.instance();
      this.player = null;
      this.source = StubSamplesSource.instance();
      this.visualizer = VisualizerStub.instance();
    }

    Holder(PlayableItem item, int samplerate) {
      this.item = item;
      this.player = item.getModule().createPlayer(samplerate);
      this.source = new SeekableSamplesSource(player);
      this.visualizer = new PlaybackVisualizer(player);
    }

    final void release() {
      if (this != instance()) {
        Analytics.sendPlayEvent(item, player);
        player.release();
        item.getModule().release();
      }
    }

    static Holder instance() {
      return Singleton.INSTANCE;
    }

    //onDemand holder idiom
    private static class Singleton {
      static final Holder INSTANCE = new Holder();
    }
  }

  private class NavigateCommand implements Command {

    private final AtomicInteger counter = new AtomicInteger(0);
    private final AtomicBoolean canceled = new AtomicBoolean(false);

    final void scheduleNext() {
      if (0 == counter.getAndIncrement()) {
        canceled.set(false);
        executeCommand(this);
      }
    }

    final void schedulePrev() {
      if (0 == counter.getAndDecrement()) {
        canceled.set(false);
        executeCommand(this);
      }
    }

    final void cancel() {
      counter.set(0);
      canceled.set(true);
    }

    @Override
    public void execute() {
      final Iterator iteratorCopy = iterator.get();
      if (performNavigation(iteratorCopy) && iterator.compareAndSet(iteratorCopy, iteratorCopy)) {
        setNewItem(iteratorCopy.getItem());
        player.startPlayback();
      }
    }

    private boolean performNavigation(Iterator iter) {
      int realDelta = 0;
      while (!canceled.get()) {
        if (counter.compareAndSet(realDelta, realDelta)) {
          break;
        }
        final int value = counter.get();
        if (value < realDelta) {
          if (iter.prev()) {
            --realDelta;
          } else {
            counter.compareAndSet(value, realDelta);
          }
        } else if (value > realDelta) {
          if (iter.next()) {
            ++realDelta;
          } else {
            counter.compareAndSet(value, realDelta);
          }
        }
      }
      counter.set(0);
      return !canceled.get() && realDelta != 0;
    }
  }

  private final class DispatchedPlaybackControl implements PlaybackControl {

    private final IteratorFactory.NavigationMode navigation;

    DispatchedPlaybackControl() {
      this.navigation = new IteratorFactory.NavigationMode(prefs);
    }

    @Override
    public void play() {
      player.startPlayback();
    }

    @Override
    public void stop() {
      executeCommand(PlaybackServiceLocal.this::stopSync);
    }

    @Override
    public void next() {
      navigateCmd.scheduleNext();
    }

    @Override
    public void prev() {
      navigateCmd.schedulePrev();
    }

    @Override
    public TrackMode getTrackMode() {
      final long val = prefs.getLong(Properties.Sound.LOOPED, 0);
      return val != 0 ? TrackMode.LOOPED : TrackMode.REGULAR;
    }

    @Override
    public void setTrackMode(TrackMode mode) {
      final long val = mode == TrackMode.LOOPED ? 1 : 0;
      prefs.putLong(Properties.Sound.LOOPED, val);
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

  private final class DispatchedSeekControl implements SeekControl {

    @Override
    public TimeStamp getDuration() {
      return holder.get().item.getDuration();
    }

    @Override
    public TimeStamp getPosition() {
      return player.getPosition();
    }

    @Override
    public void setPosition(TimeStamp position) {
      player.setPosition(position);
    }
  }

  private final class DispatchedVisualizer implements Visualizer {

    @Override
    public int getSpectrum(byte[] levels) throws Exception {
      return holder.get().visualizer.getSpectrum(levels);
    }
  }
}

