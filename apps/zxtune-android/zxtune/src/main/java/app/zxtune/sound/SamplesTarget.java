/**
 * @file
 * @brief Samples target interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import androidx.annotation.NonNull;

public interface SamplesTarget {

  /**
   * @return target sample rate in Hz
   */
  int getSampleRate();

  /**
   * @return buffer size in samples
   */
  int getPreferableBufferSize();

  /**
   * Initialize target
   */
  void start() throws Exception;

  /**
   * @param buffer sound data in S16/stereo/interleaved format
   */
  void writeSamples(@NonNull short[] buffer) throws Exception;

  /**
   * Deinitialize target
   */
  void stop() throws Exception;

  /**
   * Release all internal resources
   */
  void release();
}
