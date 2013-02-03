/*
 * @file
 * 
 * @brief Asynchronous playback support
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.sound;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Process;
import android.util.Log;
import java.util.concurrent.LinkedBlockingQueue;

public class AsyncPlayback {

  public static interface Source {
    // ! @brief Applies specified frequency rate to source
    public void setFreqRate(int freqRate);

    // ! @brief Render next sound chunk to buffer
    // ! @return false if no more sound chunks available
    public boolean getNextSoundChunk(byte[] buf);

    // ! @brief Release all captured resources
    public void release();
  }

  public static interface Callback {
    // ! @brief called on playback start
    public void onStart();

    // ! @brief called on playback stop
    public void onStop();

    // ! @brief called on playback pause
    public void onPause();

    // ! @brief called on playback resume
    public void onResume();

    // ! @brief called on playback finish
    public void onFinish();
  }

  private final static String TAG = "app.zxtune.sound.AsyncPlayback";
  private final static int DEFAULT_LATENCY = 100;// minimal is ~55

  private SyncPlayback sync;
  private Thread renderThread;
  private Thread playbackThread;

  public AsyncPlayback(Source source) {
    this(source, DEFAULT_LATENCY, AudioManager.STREAM_MUSIC);
  }

  public AsyncPlayback(Source source, int latencyMs, int streamType) {
    this.sync = new SyncPlayback(source, latencyMs, streamType);
    this.playbackThread = new Thread(new Runnable() {
      public void run() {
        try {
          Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
          sync.consumeCycle();
        } catch (InterruptedException e) {
          Log.d(TAG, "Interrupted consume cycle: " + e.getMessage());
        }
      }
    });
    this.renderThread = new Thread(new Runnable() {
      public void run() {
        try {
          Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
          sync.produceCycle();
        } catch (InterruptedException e) {
          Log.d(TAG, "Interrupted produce cycle: " + e.getMessage());
        }
      }
    });
    playbackThread.start();
    renderThread.start();
  }

  public void play() {
    sync.play();
  }

  public void pause() {
    sync.pause();
  }

  public void stop() {
    sync.stop();
    waitThread(playbackThread);
    waitThread(renderThread);
    sync.release();
    sync = null;
  }

  public void setCallback(Callback cb) {
    sync.setCallback(cb);
  }

  private static void waitThread(Thread thr) {
    try {
      thr.join();
    } catch (InterruptedException e) {}
  }

  private static class SyncPlayback {

    private static class Queue extends LinkedBlockingQueue<byte[]> {}

    private final static int CHANNELS = AudioFormat.CHANNEL_OUT_STEREO;
    private final static int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
    private final static int BUFFERS_COUNT = 2;

    private Source source;
    private AudioTrack target;
    private Callback callback;
    private Queue busyQueue;
    private Queue freeQueue;

    SyncPlayback(Source source, int latencyMs, int streamType) {
      this.source = source;
      final int freqRate = AudioTrack.getNativeOutputSampleRate(streamType);
      source.setFreqRate(freqRate);
      final int minBufSize = AudioTrack.getMinBufferSize(freqRate, CHANNELS, ENCODING);
      final int prefBufSize = 4 * (latencyMs * freqRate / 1000);
      final int bufSize = Math.max(minBufSize, prefBufSize);
      Log.d(TAG, String.format(
          "Preparing playback. Freq=%d MinBuffer=%d PrefBuffer=%d BufferSize=%d", freqRate,
          minBufSize, prefBufSize, bufSize));
      this.target =
          new AudioTrack(streamType, freqRate, CHANNELS, ENCODING, bufSize, AudioTrack.MODE_STREAM);
      this.callback = new StubCallback();
      this.busyQueue = new Queue();
      this.freeQueue = new Queue();
      for (int q = 0; q != BUFFERS_COUNT; ++q) {
        freeQueue.add(new byte[bufSize]);
      }
    }

    public void play() {
      if (isPaused()) {
        Log.d(TAG, "Resuming");
        synchronized (target) {
          target.play();
          target.notify();
        }
        callback.onResume();
      }
    }

    public void pause() {
      if (isPlaying()) {
        Log.d(TAG, "Pausing");
        synchronized (target) {
          target.pause();
        }
        callback.onPause();
      }
    }

    public void stop() {
      synchronized (target) {
        target.stop();
        target.notify();
      }
    }

    public void release() {
      target.release();
      source.release();
    }

    public void setCallback(Callback cb) {
      this.callback = cb != null ? cb : new StubCallback();
    }

    public void produceCycle() throws InterruptedException {
      Log.d(TAG, "Start producing sound data");
      callback.onStart();
      while (isActive()) {
        final byte[] buf = freeQueue.take();
        if (source.getNextSoundChunk(buf)) {
          busyQueue.put(buf);
        } else {
          callback.onFinish();
          target.stop();
        }
      }
      callback.onStop();
      Log.d(TAG, "Stop producing sound data");
    }

    public void consumeCycle() throws InterruptedException {
      target.play();
      Log.d(TAG, "Start consuming sound data");
      while (isActive()) {
        final byte[] buf = busyQueue.take();
        for (int pos = 0, toWrite = buf.length; toWrite != 0;) {
          final int written = target.write(buf, pos, toWrite);
          if (written > 0) {
            pos += written;
            toWrite -= written;
          }
          else if (written < 0 || !isActive()) {
            break;
          }
          else
          {
            synchronized (target) {
              target.wait();
            }
          }
        }
        freeQueue.put(buf);
      }
      Log.d(TAG, "Stop consuming sound data");
    }

    private boolean isPlaying() {
      return AudioTrack.PLAYSTATE_PLAYING == target.getPlayState();
    }

    private boolean isPaused() {
      return AudioTrack.PLAYSTATE_PAUSED == target.getPlayState();
    }

    private boolean isActive() {
      return AudioTrack.PLAYSTATE_STOPPED != target.getPlayState();
    }

    private static class StubCallback implements Callback {
      public void onStart() {}

      public void onStop() {}

      public void onPause() {}

      public void onResume() {}

      public void onFinish() {}
    }
  }
}
