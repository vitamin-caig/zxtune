/*
 * @file
 * @brief Broadcast implementation of Playback.Callback
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;

public final class BroadcastPlaybackCallback {

  private static final String TAG_ACTION = BroadcastPlaybackCallback.class.getName();
  private static final String TAG_ITEM = "item";
  private static final String TAG_STATUS = "status";

  public static Callback createServerStub(Context context) {
    return new ServerStub(context);
  }
  
  public static Releaseable createClientStub(Context context, Callback delegate) {
    return new ClientStub(context, delegate);
  }
  
  static final class ServerStub implements Callback {

    private final Context context;
  
    public ServerStub(Context context) {
      this.context = context;
    }
    
    @Override
    public void onItemChanged(Item item) {
      final Intent intent = new Intent(TAG_ACTION);
      intent.putExtra(TAG_ITEM, ParcelablePlaybackItem.create(item));
      context.sendBroadcast(intent);
    }
  
    @Override
    public void onStatusChanged(boolean nowPlaying) {
      final Intent intent = new Intent(TAG_ACTION);
      intent.putExtra(TAG_STATUS, nowPlaying);
      context.sendBroadcast(intent);
    }
  }
  
  static final class ClientStub implements Releaseable {
    
    private final Context context;
    private final BroadcastReceiver receiver; 
    
    public ClientStub(Context context, Callback delegate) {
      this.context = context;
      this.receiver = new Receiver(delegate);
      final IntentFilter filter = new IntentFilter(TAG_ACTION);
      this.context.registerReceiver(receiver, filter);
    }

    @Override
    public void release() {
      context.unregisterReceiver(receiver);
    }
    
    private static final class Receiver extends BroadcastReceiver {

      private final Callback delegate;
      
      public Receiver(Callback delegate) {
        this.delegate = delegate;
      }
      
      @Override
      public void onReceive(Context context, Intent intent) {
        if (intent.hasExtra(TAG_ITEM)) {
          final Item item = (Item) intent.getParcelableExtra(TAG_ITEM);
          delegate.onItemChanged(item);
        } else if (intent.hasExtra(TAG_STATUS)) {
          final boolean isPlaying = intent.getBooleanExtra(TAG_STATUS, false);
          delegate.onStatusChanged(isPlaying);
        }
      }
    }
  }
}
