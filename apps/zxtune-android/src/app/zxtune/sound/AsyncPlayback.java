/*
 * @file
 * @brief Asynchronous playback support
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.sound;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Process;
import android.util.Log;
import java.io.Closeable;
import java.io.IOException;
import java.util.concurrent.LinkedBlockingQueue;

public class AsyncPlayback {

  /**
   * Interface for sound data source
   */
  public interface Source {

    /**
     * Called on playback start
     */
    public void startup(int freqRate);
    
    /**
     * Called on playback pause
     */
    public void suspend();

    /**
     * Called on playback resume
     */
    public void resume();
    
    /**
     * Called on playback stop
     */
    public void shutdown();

    /**
     * Render next sound chunk to buffer
     * @return Is there more sound chunks available
     */
    public boolean getNextSoundChunk(byte[] buf);
  }

  private final static String TAG = AsyncPlayback.class.getName();
  private final static int DEFAULT_LATENCY = 100;// minimal is ~55

  private SyncPlayback sync;
  private final Thread renderThread;
  private final Thread playbackThread;

  public AsyncPlayback(Source source) {
    this(source, DEFAULT_LATENCY, AudioManager.STREAM_MUSIC);
  }

  public AsyncPlayback(Source source, int latencyMs, int streamType) {
    this.sync = new SyncPlayback(source, latencyMs, streamType);
    this.playbackThread = new Thread(new Runnable() {
      @Override
      public void run() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        sync.consumeCycle();
      }
    });
    this.renderThread = new Thread(new Runnable() {
      @Override
      public void run() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
        sync.produceCycle();
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
    finishThread(playbackThread);
    finishThread(renderThread);
    sync.release();
    sync = null;
  }

  private static void finishThread(Thread thr) {
    try {
      thr.interrupt();
      thr.join();
    } catch (InterruptedException e) {}
  }

  private static class SyncPlayback {

    // Typedef
    private static class Queue extends LinkedBlockingQueue<byte[]> {}

    private final static int CHANNELS = AudioFormat.CHANNEL_OUT_STEREO;
    private final static int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
    private final static int BUFFERS_COUNT = 2;

    private Source source;
    private AudioTrack target;
    private final Queue busyQueue;
    private final Queue freeQueue;

    SyncPlayback(Source source, int latencyMs, int streamType) {
      this.source = source;
      final int freqRate = AudioTrack.getNativeOutputSampleRate(streamType);
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
      if (isPlaying()) {
        Log.d(TAG, "Pausing");
        target.pause();
        source.suspend();
        Log.d(TAG, "Paused");
      }
    }

    public void resume() {
      if (isPaused()) {
        Log.d(TAG, "Resuming");
        target.play();
        source.resume();
        Log.d(TAG, "Resumed");
      }
    }

    public void stop() {
      if (isActive()) {
        Log.d(TAG, "Stopping");
        target.stop();
        Log.d(TAG, "Stopped");
      }
    }

    public void release() {
      target.release();
      target = null;
      source = null;
    }

    public void produceCycle() {
      try {
        Log.d(TAG, "Start producing sound data");
        source.startup(target.getSampleRate());
        while (isActive()) {
          final byte[] buf = freeQueue.take();
          if (source.getNextSoundChunk(buf)) {
            busyQueue.put(buf);
          } else {
            target.stop();
          }
        }
      } catch (InterruptedException e) {
        Log.d(TAG, "Interrupted producing sound data");
      } finally {
        source.shutdown();
        Log.d(TAG, "Stop producing sound data");
      }
    }

    public void consumeCycle() {
      try {
        target.play();
        Log.d(TAG, "Start consuming sound data");
        while (isActive()) {
          final byte[] buf = busyQueue.take();
          for (int pos = 0, toWrite = buf.length; toWrite != 0;) {
            final int written = target.write(buf, pos, toWrite);
            if (written > 0) {
              pos += written;
              toWrite -= written;
            } else if (written < 0 || !isActive()) {
              break;
            } else {
              synchronized (target) {
               target.wait();
              }
            }
          }
          freeQueue.put(buf);
        }
      } catch (InterruptedException e) {
        Log.d(TAG, "Interrupted consuming sound data");
      } finally {
        Log.d(TAG, "Stop consuming sound data");
      }
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
  }
}
