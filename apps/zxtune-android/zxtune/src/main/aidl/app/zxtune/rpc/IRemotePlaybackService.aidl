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

  void setNowPlaying(in Uri uri);

  //PlaylistService
  void add(in Uri[] uris);
  void delete(in long[] ids);
  void deleteAll();
  void move(in long id, in int delta);
  void sort(in String field, in String order);

  //PlaybackControl
  void play();
  void stop();
  void togglePlayStop();
  void next();
  void prev();
  int getTrackMode();
  void setTrackMode(in int val);
  int getSequenceMode();
  void setSequenceMode(in int val);
  
  //SeekControl
  long getDuration();
  long getPosition();
  void setPosition(in long ms);
   
  //Visualizer
  int[] getSpectrum();
  
  //subscription
  void subscribe(in IRemoteCallback callback);
  void unsubscribe(in IRemoteCallback callback);
}
