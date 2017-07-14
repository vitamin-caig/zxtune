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
import android.view.KeyEvent;

public class MediaButtonsHandler extends BroadcastReceiver {

  private final static String TAG = MediaButtonsHandler.class.getName();

  static Releaseable subscribe(Context context) {
    return new MediaButtonsConnection(context);
  }

  static ComponentName getName(Context context) {
    return new ComponentName(context.getApplicationContext(), MediaButtonsHandler.class);
  }

  private static class MediaButtonsConnection implements Releaseable {
    
    private final Context context;

    MediaButtonsConnection(Context context) {
      this.context = context;
      getAudioManager().registerMediaButtonEventReceiver(MediaButtonsHandler.getName(context));
    }

    @Override
    public void release() {
      getAudioManager().unregisterMediaButtonEventReceiver(MediaButtonsHandler.getName(context));
    }

    private AudioManager getAudioManager() {
      return (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    Log.d(TAG, "onReceive(intent=%s)", intent);
    final String action = getAction(intent);
    if (action != null) {
      context.startService(new Intent(action).setClass(context, MainService.class));
    }
  }

  @Nullable
  private static String getAction(Intent intent) {
    final String action = intent.getAction();
    if (Intent.ACTION_MEDIA_BUTTON.equals(action)) {
      final KeyEvent event = (KeyEvent) intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
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
      case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
        return MainService.ACTION_TOGGLE_PLAY;
      case KeyEvent.KEYCODE_MEDIA_NEXT:
        return MainService.ACTION_NEXT;
      default:
        return null;
    }
  }
}
