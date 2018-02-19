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

import android.os.Process;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.concurrent.Exchanger;
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
  private AsyncSamplesSource source;
  private SamplesTarget target;

  public SyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) throws Exception {
    this.events = events;
    this.state = new AtomicInteger(STOPPED);
    source.initialize(target.getSampleRate());
    this.source = new AsyncSamplesSource(source, target.getPreferableBufferSize());
    this.target = target;
  }

  public final void play() {
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
    }
  }

  final void pause() {
    state.compareAndSet(STARTED, PAUSING);
  }

  final void resume() {
    if (state.compareAndSet(PAUSED, RESUMING)) {
      synchronized (state) {
        state.notify();
      }
    }
  }

  final void stop() {
    if (state.compareAndSet(PAUSED, STOPPED)) {
      synchronized (state) {
        state.notify();
      }
    } else {
      state.set(STOPPED);
    }
  }

  public final void release() {
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
          final short[] buf = source.getNextSamples();
          if (buf != null) {
            target.writeSamples(buf);
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

  private static class AsyncSamplesSource {

    private static final int STOPPED = 0;
    private static final int ACTIVE = 1;
    private static final int FINISHED = 2;
    private static final int RELEASED = 3;

    private final SamplesSource source;
    private final Exchanger<short[]> exchanger;
    private short[] inputBuffer;
    private short[] outputBuffer;
    private final Thread thread;
    private final AtomicInteger state;

    AsyncSamplesSource(@NonNull SamplesSource source, int bufferSize) {
      this.source = source;
      this.exchanger = new Exchanger<>();
      this.inputBuffer = new short[bufferSize];
      this.outputBuffer = new short[bufferSize];
      this.thread = new Thread("RenderThread") {
        @Override
        public void run() {
          Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
          renderCycle();
        }
      };
      this.state = new AtomicInteger(STOPPED);
    }

    final void release() {
      try {
        if (state.compareAndSet(ACTIVE, FINISHED)) {
          thread.interrupt();
        }
        if (!state.compareAndSet(STOPPED, STOPPED)) {
          thread.join();
        }
      } catch (InterruptedException e) {
        Log.d(TAG, "Interrupted while releasing async samples source");
      } finally {
        source.release();
        state.set(RELEASED);
      }
    }

    @Nullable
    final short[] getNextSamples() throws InterruptedException {
      if (state.compareAndSet(STOPPED, ACTIVE)) {
        thread.start();
      }
      if (state.compareAndSet(ACTIVE, ACTIVE)) {
        outputBuffer = exchanger.exchange(outputBuffer);
        return outputBuffer;
      } else {
        return null;
      }
    }

    private void renderCycle() {
      try {
        while (state.compareAndSet(ACTIVE, ACTIVE)) {
          final boolean hasNextSamples = source.getSamples(inputBuffer);
          inputBuffer = exchanger.exchange(inputBuffer);
          if (!hasNextSamples) {
            break;
          }
        }
      } catch (InterruptedException e) {
      } catch (Exception e) {
      } finally {
        state.set(FINISHED);
      }
    }
  }
}
