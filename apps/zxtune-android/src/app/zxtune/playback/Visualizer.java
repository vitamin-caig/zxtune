/**
 * @file
 * @brief Declaration of visualizer interface
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

/**
 * Interface intented to get different visual-related 
 */
public interface Visualizer {

  /**
   * Get currently playing spectrum
   * @param bands array of current spectrum bands playing (usually less than 16)
   * @param levels array of current spectrum levels playing (usually less than 16)
   * @return count of actually stored values in bands/levels (less or equal of min(bands.length, levels.length))
   */
  public int getSpectrum(int[] bands, int[] levels);
  
  //public int getWaveform(byte[] waveForm) throws IllegalStateException;
}
