/*
 * @file
 * @brief Playback control remote interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;

interface IPlaybackControl {

  ParcelablePlaybackItem getItem();

  long getPlaybackPosition();
  
  int[] getSpectrumAnalysis();

  boolean isPlaying();

  void playItem(in Uri item);

  void play();

  void stop();
  
  void setPlaybackPosition(in long ms);
}
