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
   * @param levels array of current spectrum levels playing (96 max)
   * @return count of actually stored values in bands/levels (less or equal to levels.length)
   */
  int getSpectrum(byte[] levels) throws Exception;
}
