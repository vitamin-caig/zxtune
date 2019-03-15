/*
 * @file
 * @brief Remote callback interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;

oneway interface IRemoteCallback {

  void onInitialState(in int state, in ParcelablePlaybackItem item);
  void onStateChanged(in int state);
  void onItemChanged(in ParcelablePlaybackItem item);
  void onError(in String error);
}
