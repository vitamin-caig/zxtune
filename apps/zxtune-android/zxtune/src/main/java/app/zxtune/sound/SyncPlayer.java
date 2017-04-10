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

import java.util.concurrent.Exchanger;

import app.zxtune.Log;

public final class SyncPlayer implements Player {

  private static final String TAG = SyncPlayer.class.getName();
  private final PlayerEventsListener events;
  private final Exchanger<short[]> buffers;
  private SamplesSource source;
  private SamplesTarget target;
  private Thread consumeThread;
  private volatile boolean isActive;

  public SyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) {
    this.events = events;
    this.buffers = new Exchanger<short[]>();
    this.source = source;
    this.target = target;
    this.isActive = false;
  }

  @Override
  public void startPlayback() {
    try {
      startConsumeThread();
      produceCycle();
    } catch (InterruptedException e) {
      Log.w(TAG, e, "Interrupted producing sound data");
    } catch (Exception e) {
      events.onError(e);
    } finally {
      stopConsumeThread();
      events.onStop();
    }
  }

  @Override
  public void stopPlayback() {
    isActive = false;
  }
  
  @Override
  public boolean isStarted() {
    return isActive;
  }
  
  @Override
  public void release() {
    source.release();
    source = null;
    target.release();
    target = null;
  }

  private void startConsumeThread() {
    consumeThread = new ConsumeThread();
    consumeThread.start();
  }

  private void stopConsumeThread() {
    try {
      consumeThread.join();
    } catch (InterruptedException e) {
      Log.w(TAG, e, "Interrupted while stopping consume thread");
    } finally {
      consumeThread = null;
    }
  }

  private void produceCycle() throws Exception {
    try {
      Log.d(TAG, "Started produce cycle");
      source.initialize(target.getSampleRate());
      target.start();
      isActive = true;
      events.onStart();
      short[] buf = new short[target.getPreferableBufferSize()];
      while (isActive) {
        if (source.getSamples(buf)) {
          buf = buffers.exchange(buf);
        } else {
          events.onFinish();
          isActive = false;
        }
      }
    } finally {
      buffers.exchange(null);
      target.stop();
      Log.d(TAG, "Stopped produce cycle");
    }
  }

  private class ConsumeThread extends Thread {
    @Override
    public void run() {
      Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
      try {
        Log.d(TAG, "Started consume cycle");
        consumeCycle();
      } catch (InterruptedException e) {
        Log.w(TAG, e, "Interrupted consume cycle");
      } catch (Exception e) {
        events.onError(e);
        isActive = false;
      } finally {
        Log.d(TAG, "Stopped consume cycle");
      }
    }
  }

  private void consumeCycle() throws Exception {
    short[] buf = new short[target.getPreferableBufferSize()];
    for (;;) {
      buf = buffers.exchange(buf);
      if (null == buf) {
        break;
      }
      target.writeSamples(buf);
    }
  }
}
