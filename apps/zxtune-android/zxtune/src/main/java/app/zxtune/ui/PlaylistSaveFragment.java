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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import app.zxtune.analytics.Analytics;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.playlist.ProviderClient;
import app.zxtune.playlist.XspfStorage;

import java.lang.ref.WeakReference;

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
  public void onAttach(Context ctx) {
    super.onAttach(ctx);
    storage = new XspfStorage(ctx);
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
    
    private final WeakReference<Context> contextRef;
    private final XspfStorage storage;
    @Nullable private final long[] ids;
    
    SavePlaylistOperation(Context context, XspfStorage storage, @Nullable long[] ids) {
      this.contextRef = new WeakReference<>(context);
      this.storage = storage;
      this.ids = ids;
    }
    
    @Override
    @Nullable
    protected Exception doInBackground(String... name) {
      final Context context = contextRef.get();
      if (context == null) {
        return null;
      }
      try {
        final Cursor cursor = new ProviderClient(context).query(ids);
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
      final Context context = contextRef.get();
      if (context == null) {
        Log.w(TAG, new IllegalStateException(), "Expired context");
      }
      else if (error != null) {
        final String msg = context.getString(R.string.save_failed, error.getMessage());
        Toast.makeText(context, msg, Toast.LENGTH_LONG).show();
        Log.w(TAG, error, "Failed to save");
      } else {
        Toast.makeText(context, R.string.saved, Toast.LENGTH_SHORT).show();
        Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_SAVE, ids != null ? ids.length : 0);
      }
    }
  }
}
