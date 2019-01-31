package app.zxtune.sound;

import android.support.annotation.NonNull;

import java.util.concurrent.Exchanger;

import app.zxtune.Log;

class AsyncSamplesTarget {

  private static final String TAG = AsyncSamplesTarget.class.getName();

  private final SamplesTarget target;
  private final Thread thread;
  private final Exchanger<short[]> exchanger;
  private short[] inputBuffer;
  private short[] outputBuffer;

  AsyncSamplesTarget(@NonNull SamplesTarget target) {
    this.target = target;
    this.exchanger = new Exchanger<>();
    this.thread = new Thread("ConsumeSoundThread") {
      @Override
      public void run() {
        consumeCycle();
      }
    };
    final int bufferSize = target.getPreferableBufferSize();
    this.inputBuffer = new short[bufferSize];
    this.outputBuffer = new short[bufferSize];
    thread.start();
  }

  final void release() {
    while (true) {
      thread.interrupt();
      try {
        thread.join();
        break;
      } catch (InterruptedException e) {
        Log.w(TAG, new Exception(e), "Failed to release");
      }
    }
    target.release();
  }

  final void start() throws Exception {
    target.start();
  }

  final void stop() throws Exception {
    target.stop();
  }

  @NonNull
  final short[] getBuffer() throws Exception {
    final short[] result = inputBuffer;
    if (result != null) {
      return result;
    } else {
      throw new Exception("Output is in error state");
    }
  }

  final void commitBuffer() throws Exception {
    inputBuffer = exchanger.exchange(getBuffer());
  }

  private void consumeCycle() {
    try {
      while (true) {
        outputBuffer = exchanger.exchange(outputBuffer);//interruption point
        target.writeSamples(outputBuffer);
      }
    } catch (InterruptedException e) {
    } catch (Exception e) {
      Log.w(TAG, new Exception(e), "Error in consume cycle");
    } finally {
      inputBuffer = null;
      outputBuffer = null;
    }
  }
}
