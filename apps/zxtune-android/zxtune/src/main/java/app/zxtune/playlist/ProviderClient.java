package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;

public final class ProviderClient {

  private final ContentResolver resolver;

  public ProviderClient(Context ctx) {
    this.resolver = ctx.getContentResolver();
  }

  public static Uri createUri(long id) {
    return PlaylistQuery.uriFor(id);
  }

  @Nullable
  public static Long findId(Uri uri) {
    return ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())
        ? PlaylistQuery.idOf(uri)
               : null;
  }

  public final void addItem(Item item) {
    resolver.insert(PlaylistQuery.ALL, item.toContentValues());
  }

  //TODO: remove
  public final void updatePlaybackStatus(long id, boolean isPlaying) {
    resolver.update(PlaylistQuery.uriFor(id), new ItemState(isPlaying).toContentValues(), null, null);
  }

  public final void notifyChanges() {
    resolver.notifyChange(PlaylistQuery.ALL, null);
  }

  @Nullable
  public final Cursor query(@Nullable long[] ids) {
    return resolver.query(PlaylistQuery.ALL, null,
        PlaylistQuery.selectionFor(ids), null, null);
  }

  //TODO: remove
  public static Loader<Cursor> createLoader(Context ctx) {
    return new CursorLoader(ctx, PlaylistQuery.ALL, null, null, null, null);
  }

  //TODO: remove
  public static Loader<Cursor> createStatisticsLoader(Context ctx, @Nullable long[] ids) {
    return new CursorLoader(ctx, PlaylistQuery.STATISTICS, null,
        PlaylistQuery.selectionFor(ids), null, null);
  }
}
