package app.zxtune.device.media;

import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.stubs.CallbackStub;

import java.util.concurrent.TimeUnit;

//! Events gate from local service to mediasession
class StatusCallback extends CallbackStub {

  private static final String TAG = StatusCallback.class.getName();

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
