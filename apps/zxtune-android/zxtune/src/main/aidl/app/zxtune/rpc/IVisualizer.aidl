/*
 * @file
 * @brief Remote visualizer interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

interface IVisualizer {
  int getSpectrum(out byte[] levels);
}
