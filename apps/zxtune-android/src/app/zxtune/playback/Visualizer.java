/**
 *
 * @file
 *
 * @brief Visual data provider interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

public interface Visualizer {

  /**
   * Get currently playing spectrum
   * @param bands array of current spectrum bands playing (usually less than 16)
   * @param levels array of current spectrum levels playing (usually less than 16)
   * @return count of actually stored values in bands/levels (less or equal of min(bands.length, levels.length))
   */
  public int getSpectrum(int[] bands, int[] levels);
}
