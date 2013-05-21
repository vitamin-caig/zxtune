/*
 * @file
 * @brief Broadcast implementation of Playback.Callback
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import android.content.Context;
import android.content.Intent;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;

public final class BroadcastPlaybackCallback implements Callback {

  private final Context context;

  public BroadcastPlaybackCallback(Context context) {
    this.context = context;
  }
  
  @Override
  public void onControlChanged(Control control) {
  }

  @Override
  public void onItemChanged(Item item) {
    final Intent intent = new Intent(Item.class.getName());
    intent.putExtra(Item.class.getSimpleName(), ParcelablePlaybackItem.create(item));
    context.sendBroadcast(intent);
  }

  @Override
  public void onStatusChanged(boolean nowPlaying) {
    final Intent intent = new Intent(Status.class.getName());
    intent.putExtra(Status.class.getSimpleName(), nowPlaying);
    context.sendBroadcast(intent);
  }
}
