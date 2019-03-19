/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device.media;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaSessionCompat;
import app.zxtune.MainActivity;
import app.zxtune.Releaseable;
import app.zxtune.playback.service.PlaybackServiceLocal;

public class MediaSessionControl implements Releaseable {

  private static final String TAG = MediaSessionControl.class.getName();

  private final MediaSessionCompat session;
  private final Releaseable callback;

  private MediaSessionControl(Context ctx, PlaybackServiceLocal svc) {
    final Context appCtx = ctx.getApplicationContext();
    final ComponentName mbrComponent = new ComponentName(appCtx, MediaButtonReceiver.class);
    this.session = new MediaSessionCompat(ctx, TAG, mbrComponent, null);
    this.session.setFlags(MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS | MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS | MediaSessionCompat.FLAG_HANDLES_QUEUE_COMMANDS);
    this.callback = StatusCallback.subscribe(svc, session);
    this.session.setCallback(new ControlCallback(ctx, svc, session));
    this.session.setMediaButtonReceiver(PendingIntent.getBroadcast(appCtx, 0,
        new Intent(Intent.ACTION_MEDIA_BUTTON).setComponent(mbrComponent), 0));
    this.session.setSessionActivity(PendingIntent.getActivity(appCtx, 0, new Intent(appCtx, MainActivity.class), 0));
  }

  public static MediaSessionControl subscribe(Context context, PlaybackServiceLocal svc) {
    return new MediaSessionControl(context, svc);
  }

  public final MediaSessionCompat getSession() {
    return session;
  }

  @Override
  public void release() {
    session.release();
    callback.release();
  }

}
