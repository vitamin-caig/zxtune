/**
 *
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 *
 */
package app.zxtune.ui;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.widget.EditText;
import android.widget.Toast;
import app.zxtune.R;
import app.zxtune.playlist.PlaylistQuery;
import app.zxtune.playlist.XspfStorage;

class PlaylistSaveFragment extends DialogFragment {
  
  public static DialogFragment createInstance() {
    return new PlaylistSaveFragment();
  }
  
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();
    final EditText name = new EditText(ctx);
    return new AlertDialog.Builder(ctx)
      .setTitle(R.string.save)
      .setView(name)
      .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
          new SavePlaylistOperation(ctx).execute(name.getText().toString());
        }
      })
      .setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
        }
      })
      .create();
  }

  private static class SavePlaylistOperation extends AsyncTask<String, Void, Exception> {
    
    private final Context context;
    
    SavePlaylistOperation(Context context) {
      this.context = context;
    }
    
    @Override
    protected Exception doInBackground(String... name) {
      try {
        final Cursor cursor = context.getContentResolver().query(PlaylistQuery.ALL, null, null, null, null);
        final XspfStorage storage = new XspfStorage(context);
        try {
          storage.createPlaylist(name[0], cursor);
        } finally {
          cursor.close();
        }
      } catch (Exception e) {
        return e;
      }
      return null;
    }
    
    @Override
    protected void onPostExecute(Exception error) {
      if (error != null) {
        final String msg = context.getString(R.string.save_failed, error.getMessage());
        Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
      } else {
        Toast.makeText(context, R.string.saved, Toast.LENGTH_SHORT).show();
      }
    }
  }
}
