package app.zxtune.sound;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.concurrent.Exchanger;
import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Log;

class AsyncSamplesSource {

  private static final String TAG = AsyncSamplesSource.class.getName();

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
      final short[] result = exchanger.exchange(outputBuffer);
      if (result != null) {
        outputBuffer = result;
      }
      return result;
    } else {
      return null;
    }
  }

  private void renderCycle() {
    try {
      while (true) {
        final boolean hasNextSamples = source.getSamples(inputBuffer);
        inputBuffer = exchanger.exchange(inputBuffer);
        if (!hasNextSamples) {
          exchanger.exchange(null);
          source.reset();
        }
      }
    } catch (InterruptedException e) {
      Log.d(TAG, "Interrupted renderCycle");
    } catch (Exception e) {
      Log.w(TAG, e, "Unexpected exception in renderCycle");
    } finally {
      state.set(FINISHED);
    }
  }
}
