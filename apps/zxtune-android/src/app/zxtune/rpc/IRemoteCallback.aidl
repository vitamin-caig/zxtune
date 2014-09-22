/*
 * @file
 * @brief Remote callback interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;

oneway interface IRemoteCallback {

  void onStatusChanged(in boolean isPlaying);
  void onItemChanged(in ParcelablePlaybackItem item);
  void onIOStatusChanged(in boolean isActive);
}
