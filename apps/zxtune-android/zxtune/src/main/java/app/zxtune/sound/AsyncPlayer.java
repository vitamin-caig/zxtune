/**
 *
 * @file
 *
 * @brief Synchronous implementation of Player
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Log;

public final class AsyncPlayer implements Player {

  private static final String TAG = AsyncPlayer.class.getName();

  private static final int STOPPED = 0;
  private static final int STARTING = 1;
  private static final int STARTED = 2;
  private static final int PAUSING = 3;
  private static final int PAUSED = 4;
  private static final int RESUMING = 5;

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
    } else if (state.compareAndSet(PAUSED, RESUMING)) {
      synchronized (state) {
        state.notify();
      }
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
      events.onError(e);
      state.set(STOPPED);
    }
  }

  @Override
  public void pausePlayback() {
    state.compareAndSet(STARTED, PAUSING);
  }

  @Override
  public void stopPlayback() {
    if (doStop()) {
      try {
        thread.join();
      } catch (InterruptedException e) {
        Log.w(TAG, e, "Interrupted while stopping");
      } finally {
        thread = null;
      }
    } else {
      Log.d(TAG, "Already stopped");
    }
  }

  private boolean doStop() {
    synchronized (state) {
      if (state.compareAndSet(STOPPED, STOPPED)) {
        return false;
      } else if (state.compareAndSet(PAUSED, STOPPED)) {
        state.notify();
      } else {
        state.set(STOPPED);
      }
      return true;
    }
  }

  @Override
  public boolean isStarted() {
    return state.get() == STARTED;
  }

  @Override
  public boolean isPaused() {
    return state.get() == PAUSED;
  }

  @Override
  public void release() {
    source.release();
    source = null;
    target.release();
    target = null;
  }

  private void transferCycle() {
    events.onStart();
    state.set(STARTED);
    try {
      while (!state.compareAndSet(STOPPED, STOPPED)) {
        if (state.compareAndSet(PAUSING, PAUSED)) {
          doPause();
        } else {
          final short[] buf = target.getBuffer();
          if (source.getSamples(buf)) {
            target.commitBuffer();
          } else {
            events.onFinish();
            break;
          }
        }
      }
    } catch (InterruptedException e) {
      Log.d(TAG, "Interrupted transfer cycle");
    } catch (Exception e) {
      events.onError(e);
    } finally {
      state.set(STOPPED);
      events.onStop();
    }
  }

  private void doPause() throws Exception {
    target.pause();
    events.onPause();
    synchronized (state) {
      while (state.compareAndSet(PAUSED, PAUSED)) {
        state.wait();
      }
    }
    if (state.compareAndSet(RESUMING, STARTED)) {
      events.onStart();
    }
  }

}
