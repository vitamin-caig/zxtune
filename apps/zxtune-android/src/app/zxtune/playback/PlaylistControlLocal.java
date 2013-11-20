/**
 *
 * @file
 *
 * @brief Local ContentProvider-based implementation of PlaylistControl
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import app.zxtune.ScanService;
import app.zxtune.playlist.PlaylistQuery;

/**
 * Local implementation of PlaylistControl
 */
final class PlaylistControlLocal implements PlaylistControl {

  private final Context context;

  public PlaylistControlLocal(Context context) {
    this.context = context;
  }

  @Override
  public void add(Uri[] uris) {
    final Intent intent = new Intent(context, ScanService.class);
    intent.setAction(ScanService.ACTION_START);
    intent.putExtra(ScanService.EXTRA_PATHS, uris);
    context.startService(intent);
  }
  
  @Override
  public void delete(long[] ids) {
    final Uri uri = PlaylistQuery.ALL;
    final String where = PlaylistQuery.selectionFor(ids); 
    context.getContentResolver().delete(uri, where, null);
  }
  
  @Override
  public void deleteAll() {
    final Uri uri = PlaylistQuery.ALL;
    context.getContentResolver().delete(uri, null, null);
  }
}
