/*
 * @file
 * @brief Remote callback interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import app.zxtune.rpc.ParcelablePlaybackItem;

oneway interface IRemoteCallback {

  void onInitialState(in int state);
  void onStateChanged(in int state, in long pos);
  void onItemChanged(in ParcelablePlaybackItem item);
  void onError(in String error);
}
