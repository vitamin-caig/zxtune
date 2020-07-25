package app.zxtune.analytics;

import android.net.Uri;

import androidx.collection.SparseArrayCompat;

import app.zxtune.core.Player;
import app.zxtune.playback.PlayableItem;

interface Sink {

  void logException(Throwable e);

  // elapsed => tag
  void sendTrace(String id, SparseArrayCompat<String> points);

  void sendPlayEvent(PlayableItem item, Player player);

  void sendBrowserEvent(Uri path, @Analytics.BrowserAction int action);

  void sendSocialEvent(Uri path, String app, @Analytics.SocialAction int action);

  void sendUiEvent(@Analytics.UiAction int action);

  //! @param scopeSize - uris count for add, selection size else
  //TODO: cleanup
  void sendPlaylistEvent(@Analytics.PlaylistAction int action, int param);

  void sendVfsEvent(String id, String scope, @Analytics.VfsAction int action, long duration);

  void sendNoTracksFoundEvent(Uri uri);
}
