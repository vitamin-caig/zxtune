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

import java.util.Arrays;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import app.zxtune.ScanService;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.Query;

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
    //ids => '[a, b, c]'
    final String rawArgs = Arrays.toString(ids);
    final String args = rawArgs.substring(1, rawArgs.length() - 1);
    final Uri uri = Query.unparse(null);
    //placeholders doesn't work and has limitations
    final String where = Database.Tables.Playlist.Fields._id + " IN (" + args + ")"; 
    context.getContentResolver().delete(uri, where, null);
  }
  
  @Override
  public void deleteAll() {
    final Uri uri = Query.unparse(null);
    context.getContentResolver().delete(uri, null, null);
  }
}
