/*
 * @file
 * @brief Broadcast messages receiver for Playback.Callback
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.io.Closeable;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import app.zxtune.Playback;

public class BroadcastPlaybackCallbackReceiver extends BroadcastReceiver implements Closeable {

  private final Context context;
  private final Playback.Callback callback;

  public BroadcastPlaybackCallbackReceiver(Context context, Playback.Callback callback) {
    this.context = context;
    this.callback = callback;
    final IntentFilter filter = new IntentFilter();
    filter.addAction(Playback.Item.class.getName());
    filter.addAction(Playback.Status.class.getName());
    context.registerReceiver(this, filter);
  }

  @Override
  public void close() {
    context.unregisterReceiver(this);
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    final String action = intent.getAction();
    if (action.equals(Playback.Item.class.getName())) {
      final Playback.Item item =
          (Playback.Item) intent.getParcelableExtra(Playback.Item.class.getSimpleName());
      callback.itemChanged(item);
    } else if (action.equals(Playback.Status.class.getName())) {
      final Playback.Status status =
          Playback.Status.valueOf(intent.getStringExtra(Playback.Status.class.getSimpleName()));
      callback.statusChanged(status);
    }
  }
}
