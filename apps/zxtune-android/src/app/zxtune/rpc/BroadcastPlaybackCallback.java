/*
 * @file
 * @brief Broadcast implementation of Playback.Callback
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import android.content.Context;
import android.content.Intent;
import app.zxtune.Playback;

public class BroadcastPlaybackCallback implements Playback.Callback {

  private final Context context;

  public BroadcastPlaybackCallback(Context context) {
    this.context = context;
  }

  @Override
  public void itemChanged(Playback.Item item) {
    final Intent intent = new Intent(Playback.Item.class.getName());
    if (item != null) {
      intent.putExtra(Playback.Item.class.getSimpleName(), new ParcelablePlaybackItem(item));
    }
    context.sendBroadcast(intent);
  }

  @Override
  public void statusChanged(Playback.Status status) {
    final Intent intent = new Intent(Playback.Status.class.getName());
    if (status != null) {
      intent.putExtra(Playback.Status.class.getSimpleName(), new ParcelablePlaybackStatus(status));
    }
    context.sendBroadcast(intent);
  }
}
