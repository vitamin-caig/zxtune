/**
 *
 * @file
 *
 * @brief Dialog fragment with information about program
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.Dialog;
import android.content.Context;
import android.database.Cursor;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.support.v7.app.AlertDialog;
import android.widget.ArrayAdapter;
import app.zxtune.analytics.Analytics;
import app.zxtune.R;
import app.zxtune.playlist.PlaylistQuery;
import app.zxtune.playlist.Statistics;

public class PlaylistStatisticsFragment extends DialogFragment {

  private static final String IDS_KEY = "ids";
  
  public static DialogFragment createInstance(@Nullable long[] ids) {
    final DialogFragment res = new PlaylistStatisticsFragment();
    final Bundle args = new Bundle();
    args.putLongArray(IDS_KEY, ids);
    res.setArguments(args);
    Analytics.sendPlaylistEvent("Statistics", ids != null ? "selection" : "global");
    return res;
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();
    
    final ArrayAdapter<String> adapter = new ArrayAdapter<>(ctx, android.R.layout.simple_list_item_1);
    final AlertDialog result = new AlertDialog.Builder(ctx)
      .setTitle(R.string.statistics)
      .setAdapter(adapter, null)
      .create();
    setupLoader(adapter);
    return result;
  }
  
  private void setupLoader(final ArrayAdapter<String> items) {
    getLoaderManager().initLoader(0, null, new LoaderManager.LoaderCallbacks<Cursor>() {
      @Override
      public Loader<Cursor> onCreateLoader(int arg0, Bundle arg1) {
        final Bundle args = getArguments();
        final long[] ids = args != null ? args.getLongArray(IDS_KEY) : null;
        return new CursorLoader(getActivity(), PlaylistQuery.STATISTICS, null,
            PlaylistQuery.selectionFor(ids), null, null);
      }
      
      @Override
      public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
        items.clear();
        final Context ctx = getActivity();
        if (cursor.moveToFirst()) {
          final Statistics stat = new Statistics(cursor);
          final String tracks = ctx.getResources().getQuantityString(R.plurals.tracks, stat.getTotal(), stat.getTotal());
          final String duration = stat.getDuration().toString();
          items.add(ctx.getString(R.string.statistics_tracks) + ": " + tracks);
          items.add(ctx.getString(R.string.statistics_duration) + ": " + duration);
        } else {
          items.add("Failed to load...");
        }
      }

      @Override
      public void onLoaderReset(Loader<Cursor> arg0) {
      }
    });
  }
}
