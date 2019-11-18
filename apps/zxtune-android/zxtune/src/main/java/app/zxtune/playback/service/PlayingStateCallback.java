package app.zxtune.playback.service;

import android.content.Context;
import android.net.Uri;

import app.zxtune.TimeStamp;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.stubs.CallbackStub;
import app.zxtune.playlist.ProviderClient;

public class PlayingStateCallback extends CallbackStub {

  private final ProviderClient client;
  private boolean isPlaying;
  private Long playlistId;

  public PlayingStateCallback(Context context) {
    this.client = new ProviderClient(context);
    this.isPlaying = false;
    this.playlistId = null;
  }

  @Override
  public void onStateChanged(PlaybackControl.State state, TimeStamp pos) {
    final boolean isPlaying = state != PlaybackControl.State.STOPPED;
    if (this.isPlaying != isPlaying) {
      this.isPlaying = isPlaying;
      update();
    }
  }

  @Override
  public void onItemChanged(Item item) {
    final Uri newId = item.getId();
    final Long newPlaylistId = ProviderClient.findId(newId);
    if (playlistId != null && newPlaylistId == null) {
      //disable playlist item
      client.updatePlaybackStatus(playlistId, false);
    }
    playlistId = newPlaylistId;
    update();
  }

  private void update() {
    if (playlistId != null) {
      client.updatePlaybackStatus(playlistId, isPlaying);
    }
  }
}
