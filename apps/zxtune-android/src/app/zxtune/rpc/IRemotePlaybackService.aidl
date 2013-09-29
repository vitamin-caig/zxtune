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

  ParcelablePlaybackItem getNowPlaying();
  void setNowPlaying(in Uri[] uris);

  //PlaylistService
  void add(in Uri[] uris);
  void delete(in long[] ids);
  void deleteAll();

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
