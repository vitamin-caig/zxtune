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

import app.zxtune.TimeStamp;

public interface SamplesSource {

  final class Channels {
    public static final int COUNT = 2;
  }

  final class Sample {
    public static final int BYTES = 2;
  }
  
  /**
   * Acquire next sound chunk
   * @param buf result buffer of 16-bit signed interleaved stereo signal
   * @return true if buffer filled, else reset position to initial
   */
  boolean getSamples(short[] buf);

  /**
   * Synchronously changes playback position
   * @param pos
   * @throws Exception
   */
  void setPosition(TimeStamp pos);

  /**
   * Get current playback position
   * @return pos
   * @throws Exception
   */
  TimeStamp getPosition();
}
