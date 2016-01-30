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
import android.view.KeyEvent;

public class MediaButtonsHandler extends BroadcastReceiver {

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
    if (Intent.ACTION_MEDIA_BUTTON.equals(intent.getAction())) {
      final Intent toSend = getMediaButtonIntent((KeyEvent) intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT));
      if (toSend != null) {
        toSend.setClass(context, MainService.class);
        context.startService(toSend);
      }
    }
  }

  private Intent getMediaButtonIntent(KeyEvent event) {
    final int action = event.getAction();
    if (action != KeyEvent.ACTION_UP || 0 != event.getRepeatCount()) {
      return null;
    }
    switch (event.getKeyCode()) {
      case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
        return new Intent(MainService.ACTION_PREV);
      case KeyEvent.KEYCODE_MEDIA_PLAY:
        return new Intent(MainService.ACTION_PLAY);
      case KeyEvent.KEYCODE_MEDIA_PAUSE:
      case KeyEvent.KEYCODE_MEDIA_STOP:
        return new Intent(MainService.ACTION_PAUSE);
      case KeyEvent.KEYCODE_HEADSETHOOK:
      case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
        return new Intent(MainService.ACTION_PLAYPAUSE);
      case KeyEvent.KEYCODE_MEDIA_NEXT:
        return new Intent(MainService.ACTION_NEXT);
      default:
        return null;
    }
  }
}
