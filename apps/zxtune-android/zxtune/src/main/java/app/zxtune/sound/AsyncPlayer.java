/**
 * @file
 * @brief Asynchronous implementation of Player
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import androidx.annotation.Nullable;

import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.TimeStamp;

public final class AsyncPlayer implements Player {

  private static final String TAG = AsyncPlayer.class.getName();

  private static final int UNINITIALIZED = -1;
  private static final int STOPPED = 0;
  private static final int STARTING = 1;
  private static final int STARTED = 2;
  private static final int STOPPING = 3;
  private static final int FINISHING = 4;
  private static final int RELEASED = 5;

  private final PlayerEventsListener events;
  private final AtomicInteger state;
  private final AtomicReference<SamplesSource> source;
  private final AtomicReference<TimeStamp> seekRequest;
  private final AsyncSamplesTarget target;
  @Nullable
  private Thread thread;

  public static AsyncPlayer create(SamplesTarget target, PlayerEventsListener events) {
    return new AsyncPlayer(target, events);
  }

  private AsyncPlayer(SamplesTarget target, PlayerEventsListener events) {
    this.events = events;
    this.state = new AtomicInteger(UNINITIALIZED);
    this.source = new AtomicReference<>(StubSamplesSource.instance());
    this.seekRequest = new AtomicReference<>(null);
    this.target = new AsyncSamplesTarget(target);
  }

  public int getSampleRate() {
    return target.getSampleRate();
  }

  @Override
  public void setSource(SamplesSource src) {
    synchronized(state) {
      source.set(src);
      seekRequest.set(null);
      state.compareAndSet(UNINITIALIZED, STOPPED);
      state.notify();
    }
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
    } else if (state.compareAndSet(FINISHING, STARTED)) {
      //replay
      synchronized(state) {
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
      Log.w(TAG, e, "Playback initialization failed");
      events.onError(e);
      state.set(STOPPED);
    }
  }

  @Override
  public void stopPlayback() {
    if (stopping()) {
      while (thread != null) {
        try {
          thread.join();
          break;
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Interrupted while stopping");
        }
        thread.interrupt();
      }
      thread = null;
    }
  }

  private boolean stopping() {
    if (state.compareAndSet(FINISHING, STOPPING)) {
      //finish
      synchronized(state) {
        state.notify();
      }
      return true;
    } else {
      return state.compareAndSet(STARTED, STOPPING);
    }
  }

  @Override
  public boolean isStarted() {
    return state.compareAndSet(STARTED, STARTED);
  }

  public void setPosition(TimeStamp pos) {
    seekRequest.set(pos);
  }

  public TimeStamp getPosition() {
    TimeStamp res = seekRequest.get();
    if (res == null) {
      res = source.get().getPosition();
    }
    return res;
  }

  @Override
  public void release() {
    source.set(null);
    target.release();
    state.set(RELEASED);
  }

  private void transferCycle() {
    state.set(STARTED);
    events.onStart();
    try {
      while (isStarted()) {
        final SamplesSource src = source.get();
        maybeSeek(src);
        final short[] buf = target.getBuffer();
        final TimeStamp pos = src.getPosition();
        if (src.getSamples(buf)) {
          if (!commitSamples()) {
            break;
          } else if (pos.compareTo(src.getPosition()) > 0) {
            events.onStart();// looped
          }
        } else {
          events.onFinish();
          if (!waitForNextSource()) {
            Log.d(TAG, "No next source, exiting");
            break;
          }
        }
      }
    } catch (InterruptedException e) {
      if (isStarted()) {
        Log.w(TAG, e, "Interrupted transfer cycle");
      }
    } catch (Exception e) {
      events.onError(e);
    } finally {
      state.set(STOPPED);
      events.onStop();
    }
  }

  private void maybeSeek(SamplesSource src) {
    TimeStamp req = seekRequest.getAndSet(null);
    if (req != null) {
      events.onSeeking();
      while (req != null && isStarted()) {
        src.setPosition(req);
        req = seekRequest.getAndSet(null);
      }
      events.onStart();
    }
  }

  private boolean commitSamples() throws Exception {
    while (!target.commitBuffer()) {//interruption point
      if (!isStarted()) {
        return false;
      }
    }
    return true;
  }

  private boolean waitForNextSource() throws InterruptedException {
    state.set(FINISHING);
    events.onStop();
    final int NEXT_SOURCE_WAIT_SECONDS = 5;
    synchronized(state) {
      final SamplesSource prevSource = source.get();
      state.wait(NEXT_SOURCE_WAIT_SECONDS * 1000);
      if (isStarted()) {
        //replay
        events.onStart();
        return true;
      } else if (source.compareAndSet(prevSource, prevSource)) {
        return false;//same finished source
      } else {
        state.set(STARTED);
        events.onStart();
        return true;
      }
    }
  }
}
