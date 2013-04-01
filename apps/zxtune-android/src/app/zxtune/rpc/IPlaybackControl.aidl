/*
 * @file
 * @brief Playback control remote interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;
import app.zxtune.rpc.ParcelablePlaybackStatus;

interface IPlaybackControl {

  ParcelablePlaybackItem getItem();

  ParcelablePlaybackStatus getStatus();

  void playItem(in Uri item);

  void play();

  void pause();

  void stop();
}
