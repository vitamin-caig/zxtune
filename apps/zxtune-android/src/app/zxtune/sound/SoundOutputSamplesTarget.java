/**
 *
 * @file
 *
 * @brief Playback implementation of SamplesTarget based on AudioTrack functionality
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

final class SoundOutputSamplesTarget implements SamplesTarget {

  private static final String TAG = SoundOutputSamplesTarget.class.getName();
  
  private static final class Channels {
    public final static int COUNT = 2;
    public final static int ENCODING = AudioFormat.CHANNEL_OUT_STEREO;
  }
  private static final class Sample {
    public final static int BYTES = 2;
    public final static int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
  }
  private final static int STREAM = AudioManager.STREAM_MUSIC;
  private final static int DEFAULT_LATENCY = 80;// minimal is ~55
  
  private final int bufferSize;
  private AudioTrack target;
  
  private SoundOutputSamplesTarget(int bufferSize, AudioTrack target) {
    this.bufferSize = bufferSize;
    this.target = target;
  }
  
  public static SamplesTarget create() {
    final int freqRate = AudioTrack.getNativeOutputSampleRate(STREAM);
    final int minBufSize = AudioTrack.getMinBufferSize(freqRate, Channels.ENCODING, Sample.ENCODING);
    final int prefBufSize = Channels.COUNT * Sample.BYTES * (DEFAULT_LATENCY * freqRate / 1000);
    final int bufSize = Math.max(minBufSize, prefBufSize);
    Log.d(TAG, String.format(
        "Preparing playback. Freq=%d MinBuffer=%d PrefBuffer=%d BufferSize=%d", freqRate,
        minBufSize, prefBufSize, bufSize));
    final AudioTrack target =
        new AudioTrack(STREAM, freqRate, Channels.ENCODING, Sample.ENCODING, bufSize, AudioTrack.MODE_STREAM);
    return new SoundOutputSamplesTarget(bufSize, target);
  }
  
  @Override
  public int getSampleRate() {
    return target.getSampleRate();
  }
  
  @Override
  public int getPreferableBufferSize() {
    return bufferSize / Sample.BYTES;
  }
  
  @Override
  public void start() {
    assert target.getPlayState() == AudioTrack.PLAYSTATE_STOPPED;
    target.play();
  }

  @Override
  public void writeSamples(short[] buffer) {
    for (int pos = 0, toWrite = buffer.length; toWrite != 0;) {
      final int written = target.write(buffer, pos, toWrite);
      if (written > 0) {
        pos += written;
        toWrite -= written;
      } else {
        throw new RuntimeException("Failed to write samples: " + written);
      }
    }
  }

  @Override
  public void stop() {
    assert target.getPlayState() == AudioTrack.PLAYSTATE_PLAYING;
    target.stop();
  }

  @Override
  public void release() {
    target.release();
    target = null;
  }
}
