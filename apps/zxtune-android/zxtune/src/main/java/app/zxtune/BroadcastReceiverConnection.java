/**
 *
 * @file
 *
 * @brief Broadcast receiver connection handler helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;

public class BroadcastReceiverConnection implements Releaseable {
  
  protected final Context context;
  private final BroadcastReceiver receiver;
  
  public BroadcastReceiverConnection(Context context, BroadcastReceiver receiver, IntentFilter filter) {
    this.context = context;
    this.receiver = receiver;
    context.registerReceiver(receiver, filter);
    Log.d(receiver.getClass().getName(), "Registered");
  }
  
  @Override
  public void release() {
    context.unregisterReceiver(receiver);
    Log.d(receiver.getClass().getName(), "Unregistered");
  }
}
