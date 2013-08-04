/*
 * @file
 * @brief Playback service remote interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;
import app.zxtune.rpc.IRemoteCallback;

interface IRemotePlaybackService {

  //PlaybackService
  ParcelablePlaybackItem getNowPlaying();
  void setNowPlaying(in Uri uri);

  //PlaybackControl
  void play();
  void stop();
  boolean isPlaying();
  void next();
  void prev();
  
  //SeekControl
  long getDuration();
  long getPosition();
  void setPosition(in long ms);
   
  //Visualizer
  int[] getSpectrum();
  
  //subscription
  void subscribe(IRemoteCallback callback);
  void unsubscribe(IRemoteCallback callback);
}
