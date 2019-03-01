/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.stubs.CallbackStub;

import java.util.concurrent.TimeUnit;

class RemoteControl implements Releaseable {

  private static final String TAG = RemoteControl.class.getName();

  private final MediaSessionCompat session;
  private final Releaseable callback;

  private RemoteControl(Context ctx, PlaybackService svc) {
    final Context appCtx = ctx.getApplicationContext();
    final ComponentName mbrComponent = new ComponentName(appCtx, MediaButtonReceiver.class);
    this.session = new MediaSessionCompat(ctx, TAG, mbrComponent, null);
    this.session.setFlags(MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS | MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);
    this.callback = new CallbackSubscription(svc, new StatusCallback(session));
    this.session.setCallback(new ControlCallback(svc.getPlaybackControl()));
    this.session.setMediaButtonReceiver(PendingIntent.getBroadcast(appCtx, 0,
        new Intent(Intent.ACTION_MEDIA_BUTTON).setComponent(mbrComponent), 0));
    this.session.setSessionActivity(PendingIntent.getActivity(appCtx, 0, new Intent(appCtx, MainActivity.class), 0));
  }

  public static RemoteControl subscribe(Context context, PlaybackService svc) {
    return new RemoteControl(context, svc);
  }

  final MediaSessionCompat getSession() {
    return session;
  }

  @Override
  public void release() {
    session.release();
    callback.release();
  }

  private static class StatusCallback extends CallbackStub {

    private final MediaSessionCompat session;
    private final PlaybackStateCompat.Builder builder;

    StatusCallback(MediaSessionCompat session) {
      this.session = session;
      this.builder = new PlaybackStateCompat.Builder();
      this.builder.setActions(
          PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS | PlaybackStateCompat.ACTION_PLAY_PAUSE |
              PlaybackStateCompat.ACTION_PLAY | PlaybackStateCompat.ACTION_PAUSE |
              PlaybackStateCompat.ACTION_STOP | PlaybackStateCompat.ACTION_SKIP_TO_NEXT
      );
    }

    @Override
    public void onInitialState(PlaybackControl.State state, Item item, boolean ioStatus) {
      onStateChanged(state);
    }

    @Override
    public void onStateChanged(PlaybackControl.State state) {
      final boolean isPlaying = state == PlaybackControl.State.PLAYING;
      final int stateCompat = isPlaying
                                  ? PlaybackStateCompat.STATE_PLAYING
                                  : PlaybackStateCompat.STATE_STOPPED;
      builder.setState(stateCompat, -1, 1);
      session.setPlaybackState(builder.build());
      session.setActive(isPlaying);
    }

    @Override
    public void onItemChanged(Item item) {
      try {
        final Identifier dataId = item.getDataId();
        final MediaMetadataCompat.Builder builder = new MediaMetadataCompat.Builder();
        final String author = item.getAuthor();
        final String title = item.getTitle();
        final boolean noAuthor = author.length() == 0;
        final boolean noTitle = title.length() == 0;
        if (noAuthor && noTitle) {
          final String filename = dataId.getDisplayFilename();
          builder.putString(MediaMetadataCompat.METADATA_KEY_TITLE, filename);
        } else {
          builder.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, author);
          builder.putString(MediaMetadataCompat.METADATA_KEY_ALBUM_ARTIST, author);
          builder.putString(MediaMetadataCompat.METADATA_KEY_TITLE, title);
        }
        builder.putLong(MediaMetadataCompat.METADATA_KEY_DURATION,
            item.getDuration().convertTo(TimeUnit.MILLISECONDS));
        session.setMetadata(builder.build());
      } catch (Exception e) {
        Log.w(TAG, e, "onItemChanged()");
      }
    }
  }

  private static class ControlCallback extends MediaSessionCompat.Callback {

    private final PlaybackControl ctrl;

    ControlCallback(PlaybackControl ctrl) {
      this.ctrl = ctrl;
    }

    @Override
    public void onPlay() {
      ctrl.play();
    }

    @Override
    public void onPause() {
      ctrl.stop();
    }

    @Override
    public void onStop() {
      ctrl.stop();
    }

    @Override
    public void onSkipToPrevious() {
      ctrl.prev();
    }

    @Override
    public void onSkipToNext() {
      ctrl.next();
    }
  }
}
