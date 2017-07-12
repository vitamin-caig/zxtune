/**
 *
 * @file
 *
 * @brief Samples target interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

public interface SamplesTarget {

  /**
   * @return target sample rate in Hz
   */
  public int getSampleRate();
  
  /**
   * @return buffer size in samples
   */
  public int getPreferableBufferSize();

  /**
   * Initialize target
   */
  public void start() throws Exception;

  /**
   * Pause target. No underrun happens if no data called.
   * Mode is active till next writeSamples or stop call
   */
  public void pause() throws Exception;

  /**
   * @param buffer sound data in S16/stereo/interleaved format
   */
  public void writeSamples(short[] buffer) throws Exception;

  /**
   * Deinitialize target
   */
  public void stop() throws Exception;
  
  /**
   * Release all internal resources
   */
  public void release();
}
