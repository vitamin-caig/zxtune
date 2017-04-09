/**
 *
 * @file
 *
 * @brief Samples source interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

public interface SamplesSource {

  public static final class Channels {
    public static final int COUNT = 2;
  }

  public static final class Sample {
    public static final int BYTES = 2;
  }
  
  /**
   * Initialize stream for input
   * @param sampleRate required sample rate in Hz (e.g. 44100)
   */
  public void initialize(int sampleRate);
  
  /**
   * Acquire next sound chunk
   * @param buf result buffer of 16-bit signed interleaved stereo signal
   * @return true if buffer filled
   */
  public boolean getSamples(short[] buf);
  
  /**
   * Release all internal resources
   */
  public void release();
}
