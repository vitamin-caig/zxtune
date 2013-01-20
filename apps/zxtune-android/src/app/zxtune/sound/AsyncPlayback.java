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

  private final static String TAG = "app.zxtune.sound.AsyncPlayback";
  private final static int DEFAULT_LATENCY = 200; 

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
          sync.consumeCycle();
        } catch (InterruptedException e) {}
      }
    });
    this.renderThread = new Thread(new Runnable() {
      public void run() {
        try {
          sync.produceCycle();
        } catch (InterruptedException e) {}
      }
    });
    playbackThread.start();
    renderThread.start();
  }

  public void pause() {
    sync.pause();
  }

  public void resume() {
    sync.resume();
  }

  public void stop() {
    sync.stop();
    waitThread(playbackThread);
    waitThread(renderThread);
    sync.release();
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
      this.busyQueue = new Queue();
      this.freeQueue = new Queue();
      for (int q = 0; q != BUFFERS_COUNT; ++q) {
        freeQueue.add(new byte[bufSize]);
      }
    }

    public void pause() {
      target.pause();
    }

    public void resume() {
      target.play();
    }

    public void stop() {
      target.stop();
    }

    public void release() {
      target.release();
      source.release();
    }

    public void produceCycle() throws InterruptedException {
      Log.d(TAG, "Start producing sound data");
      while (isPlaying()) {
        final byte[] buf = freeQueue.take();
        if (source.getNextSoundChunk(buf)) {
          busyQueue.put(buf);
        }
      }
      Log.d(TAG, "Stop producing sound data");
    }

    public void consumeCycle() throws InterruptedException {
      target.play();
      Log.d(TAG, "Start consuming sound data");
      while (isPlaying()) {
        final byte[] buf = busyQueue.take();
        target.write(buf, 0, buf.length);
        freeQueue.put(buf);
      }
      Log.d(TAG, "Stop consuming sound data");
    }

    private boolean isPlaying() {
      return target.getPlayState() != AudioTrack.PLAYSTATE_STOPPED;
    }
  }
}
