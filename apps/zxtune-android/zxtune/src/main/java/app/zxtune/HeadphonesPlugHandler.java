/**
 *
 * @file
 *
 * @brief Handling headphones unplug event
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import app.zxtune.playback.PlaybackControl;

class HeadphonesPlugHandler extends BroadcastReceiver {

  private final static String STATE_FIELD_NAME = "state";
  private final static int STATE_UNPLUGGED = 0;
  private final static int STATE_PLUGGED = 1;
  
  private final PlaybackControl control;

  private HeadphonesPlugHandler(PlaybackControl control) {
    this.control = control;
  }
  
  static Releaseable subscribe(Context context, PlaybackControl control) {
    final BroadcastReceiver handler = new HeadphonesPlugHandler(control);
    final IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
    filter.setPriority(Integer.MAX_VALUE);
    return new BroadcastReceiverConnection(context, handler, filter);
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    if (!intent.hasExtra(STATE_FIELD_NAME)) {
      return;
    }
    switch (intent.getIntExtra(STATE_FIELD_NAME, STATE_UNPLUGGED)) {
      case STATE_PLUGGED:
        break;
      case STATE_UNPLUGGED:
        if (control.isPlaying()) {
          control.stop();
        }
    }
  }
}
