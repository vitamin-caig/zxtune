/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.view.KeyEvent;
import app.zxtune.playback.PlaybackControl;

class MediaButtonsHandler extends BroadcastReceiver {
  
  private final PlaybackControl control;

  private MediaButtonsHandler(PlaybackControl control) {
    this.control = control;
  }
  
  static Releaseable subscribe(Context context, PlaybackControl control) {
    final BroadcastReceiver handler = new MediaButtonsHandler(control);
    final IntentFilter filter = new IntentFilter(Intent.ACTION_MEDIA_BUTTON);
    filter.setPriority(Integer.MAX_VALUE);
    return new BroadcastReceiverConnection(context, handler, filter);
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    if (Intent.ACTION_MEDIA_BUTTON.equals(intent.getAction())) {
      processMediaButton((KeyEvent) intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT));
    }
  }

  private void processMediaButton(KeyEvent event) {
    final int action = event.getAction();
    if (action != KeyEvent.ACTION_UP) {
      return;
    }
    switch (event.getKeyCode()) {
      case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
        control.prev();
        break;
      case KeyEvent.KEYCODE_HEADSETHOOK:
      case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
        if (control.isPlaying()) {
          control.stop();
        } else {
          control.play();
        }
        break;
      case KeyEvent.KEYCODE_MEDIA_NEXT:
        control.next();
        break;
    }
  }
}
