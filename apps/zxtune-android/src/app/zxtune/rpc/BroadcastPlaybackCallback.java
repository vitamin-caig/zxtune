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
    intent.putExtra(Playback.Item.class.getSimpleName(), ParcelablePlaybackItem.create(item));
    context.sendBroadcast(intent);
  }

  @Override
  public void statusChanged(Playback.Status status) {
    final Intent intent = new Intent(Playback.Status.class.getName());
    intent.putExtra(Playback.Status.class.getSimpleName(), status.name());
    context.sendBroadcast(intent);
  }
}
