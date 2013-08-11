/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.util.Log;

public class BroadcastReceiverConnection implements Releaseable {
  
  private final Context context;
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
