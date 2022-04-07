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
import android.os.Build;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;

import androidx.media.session.MediaButtonReceiver;

import app.zxtune.MainActivity;
import app.zxtune.Releaseable;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.service.PlaybackServiceLocal;
import app.zxtune.rpc.ParcelableBinder;
import app.zxtune.rpc.VisualizerProxy;

public class MediaSessionControl implements Releaseable {

  private static final String TAG = MediaSessionControl.class.getName();

  private final MediaSessionCompat session;
  private final Releaseable callback;

  private MediaSessionControl(Context ctx, PlaybackServiceLocal svc) {
    final ComponentName mbrComponent = new ComponentName(ctx, MediaButtonReceiver.class);
    session = new MediaSessionCompat(ctx, TAG, mbrComponent, null);
    session.setFlags(MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS | MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS | MediaSessionCompat.FLAG_HANDLES_QUEUE_COMMANDS);
    callback = StatusCallback.subscribe(ctx, svc, session);
    session.setCallback(new ControlCallback(ctx, svc, session));
    session.setMediaButtonReceiver(PendingIntent.getBroadcast(ctx, 0,
        new Intent(Intent.ACTION_MEDIA_BUTTON).setComponent(mbrComponent),
        MainActivity.PENDING_INTENT_FLAG));
    session.setSessionActivity(MainActivity.createPendingIntent(ctx));
    session.setExtras(createExtras(svc));
  }

  private static Bundle createExtras(PlaybackServiceLocal svc) {
    final Bundle extras = new Bundle();
    if (Build.VERSION.SDK_INT >= 18) {
      extras.putBinder(Visualizer.class.getName(), VisualizerProxy.getServer(svc.getVisualizer()));
    } else {
      extras.putParcelable(Visualizer.class.getName(), ParcelableBinder.serialize(VisualizerProxy.getServer(svc.getVisualizer())));
    }
    return extras;
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
