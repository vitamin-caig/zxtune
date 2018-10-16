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

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.playlist.PlaylistQuery;
import app.zxtune.playlist.XspfStorage;

public class PlaylistSaveFragment extends DialogFragment {

  private static final String TAG = PlaylistSaveFragment.class.getName();
  private static final String IDS_KEY = "ids";
  private static final String NAME_KEY = "name";

  private XspfStorage storage;
  private EditText name;
  
  public static DialogFragment createInstance(@Nullable long[] ids) {
    final DialogFragment res = new PlaylistSaveFragment();
    final Bundle args = new Bundle();
    args.putLongArray(IDS_KEY, ids);
    res.setArguments(args);
    return res;
  }
  
  @Override
  public void onAttach(Activity act) {
    super.onAttach(act);
    storage = new XspfStorage(act);
  }
  
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();
    
    name = createEditText(ctx);
    final AlertDialog result = new AlertDialog.Builder(ctx)
      .setTitle(R.string.save)
      .setView(name)
      .setPositiveButton(R.string.save, new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
          final Bundle args = getArguments();
          final long[] ids = args != null ? args.getLongArray(IDS_KEY) : null;
          new SavePlaylistOperation(ctx, storage, ids)
            .execute(name.getText().toString());
        }
      })
      .create();
    if (savedInstanceState != null && savedInstanceState.containsKey(NAME_KEY)) {
      name.onRestoreInstanceState(savedInstanceState.getParcelable(NAME_KEY));
    }
    return result;
  }
  
  @Override
  public void onStart() {
    super.onStart();
    //buttons are shown now
    name.setText(name.getText());
  }
  
  private EditText createEditText(Context ctx) {
    final EditText result = new EditText(ctx);
    result.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
    result.setSingleLine();
    
    result.addTextChangedListener(new TextWatcher() {
      
      private Button okButton;

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        if (okButton == null) {
          final AlertDialog dlg = (AlertDialog) getDialog();
          if (dlg != null) {
            okButton = dlg.getButton(DialogInterface.BUTTON_POSITIVE);
          }
        }
      }

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count) {
      }

      @Override
      public void afterTextChanged(Editable s) {
        if (s == null || s.length() == 0) {
          textEmpty();
        } else {
          textChanged(s.toString());
        }
      }
      
      private void textEmpty() {
        if (okButton != null) {
          okButton.setEnabled(false);
        }
      }
      
      private void textChanged(String content) {
        if (okButton != null) {
          okButton.setEnabled(true);
          okButton.setText(storage.isPlaylistExists(content) ? R.string.overwrite : R.string.save);
        }
      }
    });
    return result;
  }
  
  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    outState.putParcelable(NAME_KEY, name.onSaveInstanceState());
  }
  
  private static class SavePlaylistOperation extends AsyncTask<String, Void, Exception> {
    
    private final Context context;
    private final XspfStorage storage;
    @Nullable private final long[] ids;
    
    SavePlaylistOperation(Context context, XspfStorage storage, @Nullable long[] ids) {
      this.context = context;
      this.storage = storage;
      this.ids = ids;
    }
    
    @Override
    @Nullable
    protected Exception doInBackground(String... name) {
      try {
        final Cursor cursor = context.getContentResolver().query(PlaylistQuery.ALL, null, 
          PlaylistQuery.selectionFor(ids), null, null);
        if (cursor == null) {
          return new Exception("Failed to query playlist");
        }
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
        Log.w(TAG, error, "Failed to save");
      } else {
        Toast.makeText(context, R.string.saved, Toast.LENGTH_SHORT).show();
        Analytics.sendPlaylistEvent("Save", ids != null ? "selection" : "all");
      }
    }
  }
}
