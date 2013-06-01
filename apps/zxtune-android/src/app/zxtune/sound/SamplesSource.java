/**
 * @file
 * @brief Interface for sound samples source declaration
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

import app.zxtune.TimeStamp;

/**
 * Abstract sound samples source
 */
public interface SamplesSource {

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
   * @param pos Absolute time position to seek  
   */
  public void setPosition(TimeStamp pos);
  
  /**
   * @return Absolute time position of playback
   */
  public TimeStamp getPosition();
  /**
   * Release all internal resources
   */
  public void release();
}
