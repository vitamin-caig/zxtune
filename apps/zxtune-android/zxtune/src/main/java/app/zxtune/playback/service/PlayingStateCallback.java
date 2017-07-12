package app.zxtune.playback.service;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

import app.zxtune.playback.stubs.CallbackStub;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playlist.ItemState;
import app.zxtune.playlist.PlaylistQuery;

class PlayingStateCallback extends CallbackStub {

    private final ContentResolver resolver;
    private boolean isPlaying;
    private Uri dataLocation;
    private Long playlistId;

    PlayingStateCallback(Context context) {
        this.resolver = context.getContentResolver();
        this.isPlaying = false;
        this.dataLocation = Uri.EMPTY;
        this.playlistId = null;
    }

    @Override
    public void onStateChanged(PlaybackControl.State state) {
        final boolean isPlaying = state != PlaybackControl.State.STOPPED;
        if (this.isPlaying != isPlaying) {
            this.isPlaying = isPlaying;
            update();
        }
    }

    @Override
    public void onItemChanged(Item item) {
        final Uri newId = item.getId();
        final Uri newDataLocation = item.getDataId().getFullLocation();
        final Long newPlaylistId = 0 == newId.compareTo(newDataLocation) ? null : PlaylistQuery.idOf(newId);
        if (playlistId != null && newPlaylistId == null) {
            //disable playlist item
            updatePlaylist(playlistId, false);
        }
        playlistId = newPlaylistId;
        dataLocation = newDataLocation;
        update();
    }

    private void update() {
        if (playlistId != null) {
            updatePlaylist(playlistId, isPlaying);
        }
    }

    private void updatePlaylist(long id, boolean isPlaying) {
        resolver.update(PlaylistQuery.uriFor(id), new ItemState(isPlaying).toContentValues(), null, null);
    }
}
