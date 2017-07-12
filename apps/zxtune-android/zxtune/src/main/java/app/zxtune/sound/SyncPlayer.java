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

public final class SyncPlayer {

  private static final String TAG = SyncPlayer.class.getName();

  private static final int STOPPED = 0;
  private static final int STARTED = 1;
  private static final int PAUSING = 2;
  private static final int PAUSED = 3;
  private static final int RESUMING = 4;

  private final PlayerEventsListener events;
  private final AtomicInteger state;
  private SamplesSource source;
  private SamplesTarget target;

  public SyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) {
    this.events = events;
    this.state = new AtomicInteger(STOPPED);
    this.source = source;
    this.target = target;
  }

  public final void play() {
    try {
      source.initialize(target.getSampleRate());
      final short[] buf = new short[target.getPreferableBufferSize()];
      target.start();
      try {
        Log.d(TAG, "Start transfer cycle");
        transferCycle(buf);
      } finally {
        Log.d(TAG, "Stop transfer cycle");
        target.stop();
      }
    } catch (Exception e) {
      events.onError(e);
    }
  }

  public final void pause() {
    state.compareAndSet(STARTED, PAUSING);
  }

  public final void resume() {
    if (state.compareAndSet(PAUSED, RESUMING)) {
      synchronized (state) {
        state.notify();
      }
    }
  }

  public final void stop() {
    if (state.compareAndSet(PAUSED, STOPPED)) {
      synchronized (state) {
        state.notify();
      }
    } else {
      state.set(STOPPED);
    }
  }

  public void release() {
    source.release();
    source = null;
    target.release();
    target = null;
  }

  private void transferCycle(short[] buf) {
    events.onStart();
    state.set(STARTED);
    try {
      while (!state.compareAndSet(STOPPED, STOPPED)) {
        if (state.compareAndSet(PAUSING, PAUSED)) {
          doPause();
        } else if (source.getSamples(buf)) {
          target.writeSamples(buf);
        } else {
          events.onFinish();
          break;
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

  private void doPause() throws InterruptedException {
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
