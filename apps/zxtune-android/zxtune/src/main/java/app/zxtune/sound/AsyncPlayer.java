/**
 * @file
 * @brief Synchronous implementation of Player
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Log;

public final class AsyncPlayer implements Player {

  private static final String TAG = AsyncPlayer.class.getName();

  private static final int STOPPED = 0;
  private static final int STARTING = 1;
  private static final int STARTED = 2;
  private static final int STOPPING = 3;

  private final PlayerEventsListener events;
  private final AtomicInteger state;
  private SamplesSource source;
  private AsyncSamplesTarget target;
  private Thread thread;

  public static Player create(SamplesSource source, SamplesTarget target, PlayerEventsListener events) throws Exception {
    return new AsyncPlayer(source, target, events);
  }

  private AsyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) throws Exception {
    this.events = events;
    this.state = new AtomicInteger(STOPPED);
    source.initialize(target.getSampleRate());
    this.source = source;
    this.target = new AsyncSamplesTarget(target);
  }

  @Override
  public void startPlayback() {
    if (state.compareAndSet(STOPPED, STARTING)) {
      thread = new Thread("PlayerThread") {
        @Override
        public void run() {
          syncPlay();
        }
      };
      thread.start();
    }
  }

  private void syncPlay() {
    try {
      target.start();
      try {
        Log.d(TAG, "Start transfer cycle");
        transferCycle();
      } finally {
        Log.d(TAG, "Stop transfer cycle");
        target.stop();
      }
    } catch (Exception e) {
      Log.w(TAG, new Exception(e), "Playback initialization failed");
      events.onError(e);
      state.set(STOPPED);
    }
  }

  @Override
  public void stopPlayback() {
    if (state.compareAndSet(STARTED, STOPPING)) {
      while (true) {
        thread.interrupt();
        try {
          thread.join();
          break;
        } catch (InterruptedException e) {
          Log.w(TAG, new Exception(e), "Interrupted while stopping");
        }
      }
      thread = null;
      state.set(STOPPED);
    }
  }

  @Override
  public boolean isStarted() {
    return state.compareAndSet(STARTED, STARTED);
  }

  @Override
  public void release() {
    source = null;
    target.release();
    target = null;
  }

  private void transferCycle() {
    state.set(STARTED);
    events.onStart();
    try {
      while (isStarted()) {
        final short[] buf = target.getBuffer();
        if (source.getSamples(buf)) {
          target.commitBuffer();//interruption point
        } else {
          events.onFinish();
          break;
        }
      }
    } catch (InterruptedException e) {
      if (isStarted()) {
        Log.w(TAG, new Exception(e),"Interrupted transfer cycle");
      }
    } catch (Exception e) {
      events.onError(e);
    } finally {
      events.onStop();
    }
  }
}
