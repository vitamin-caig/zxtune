/**
 *
 * @file
 *
 * @brief Handling media button events
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.view.KeyEvent;

public class MediaEventsHandler extends BroadcastReceiver {

  private static final String TAG = MediaEventsHandler.class.getName();

  public static ComponentName getName(Context context) {
    return new ComponentName(context.getApplicationContext(), MediaEventsHandler.class);
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    Log.d(TAG, "onReceive(intent=%s)", intent);
    final String action = getAction(intent);
    if (action != null) {
      ContextCompat.startForegroundService(context, MainService.createIntent(context, action));
    }
  }

  @Nullable
  private static String getAction(Intent intent) {
    final String action = intent.getAction();
    if (Intent.ACTION_MEDIA_BUTTON.equals(action)) {
      final KeyEvent event = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
      return getAction(event);
    } else if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(action)) {
      return MainService.ACTION_PAUSE;
    } else {
      return null;
    }
  }

  @Nullable
  private static String getAction(@Nullable KeyEvent event) {
    if (event == null) {
      return null;
    }
    final int action = event.getAction();
    if (action != KeyEvent.ACTION_UP || 0 != event.getRepeatCount()) {
      return null;
    }
    switch (event.getKeyCode()) {
      case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
        return MainService.ACTION_PREV;
      case KeyEvent.KEYCODE_MEDIA_PLAY:
        return MainService.ACTION_PLAY;
      case KeyEvent.KEYCODE_MEDIA_PAUSE:
        return MainService.ACTION_PAUSE;
      case KeyEvent.KEYCODE_MEDIA_STOP:
        return MainService.ACTION_STOP;
      case KeyEvent.KEYCODE_HEADSETHOOK:
        //else another player may steal sound session and control
        return MainService.ACTION_TOGGLE_PLAY_PAUSE;
      case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
        return MainService.ACTION_TOGGLE_PLAY_PAUSE;
      case KeyEvent.KEYCODE_MEDIA_NEXT:
        return MainService.ACTION_NEXT;
      default:
        return null;
    }
  }
}
