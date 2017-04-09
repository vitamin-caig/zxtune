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

import app.zxtune.Log;
import app.zxtune.sound.SamplesSource.Channels;
import app.zxtune.sound.SamplesSource.Sample;

public final class SoundOutputSamplesTarget implements SamplesTarget {

  private static final String TAG = SoundOutputSamplesTarget.class.getName();
  
  private static final int CHANNEL_OUT = AudioFormat.CHANNEL_OUT_STEREO;
  private static final int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
  private static final int STREAM = AudioManager.STREAM_MUSIC;
  private static final int DEFAULT_LATENCY = 80;// minimal is ~55
  
  private final int bufferSize;
  private AudioTrack target;
  
  private SoundOutputSamplesTarget(int bufferSize, AudioTrack target) {
    this.bufferSize = bufferSize;
    this.target = target;
  }
  
  public static SamplesTarget create() {
    //xmp plugin limits max frequency
    final int freqRate = Math.min(AudioTrack.getNativeOutputSampleRate(STREAM), 48000);
    final int minBufSize = AudioTrack.getMinBufferSize(freqRate, CHANNEL_OUT, ENCODING);
    final int prefBufSize = Channels.COUNT * Sample.BYTES * (DEFAULT_LATENCY * freqRate / 1000);
    final int bufSize = Math.max(minBufSize, prefBufSize);
    Log.d(TAG, "Preparing playback. Freq=%d MinBuffer=%d PrefBuffer=%d BufferSize=%d", freqRate,
        minBufSize, prefBufSize, bufSize);
    final AudioTrack target =
        new AudioTrack(STREAM, freqRate, CHANNEL_OUT, ENCODING, bufSize, AudioTrack.MODE_STREAM);
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
    target.stop();
  }

  @Override
  public void release() {
    target.release();
    target = null;
  }
}
