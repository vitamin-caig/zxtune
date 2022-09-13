/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.playlist;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.LiveData;

import java.util.Set;
import java.util.concurrent.Executors;

import app.zxtune.R;
import app.zxtune.device.PersistentStorage;
import app.zxtune.playlist.ProviderClient;
import app.zxtune.ui.PersistentStorageSetupFragment;

public class PlaylistSaveFragment extends DialogFragment {

  private static final String IDS_KEY = "ids";
  private static final String NAME_KEY = "name";

  @Nullable
  private ProviderClient client;
  @Nullable
  private Set<String> usedNames;
  @Nullable
  private EditText name;

  static void show(Fragment parent, PersistentStorage storage, @Nullable long[] ids) {
    final LifecycleOwner owner = parent.getViewLifecycleOwner();
    final LiveData<Intent> setupIntent = storage.getSetupIntent();
    setupIntent.observe(owner, (@Nullable Intent intent) -> {
      setupIntent.removeObservers(owner);
      final DialogFragment result = intent != null ?
          PersistentStorageSetupFragment.createInstance(intent) : createInstance(ids);
      result.show(parent.getParentFragmentManager(), "save_playlist");
    });
  }

  private static DialogFragment createInstance(@Nullable long[] ids) {
    final DialogFragment res = new PlaylistSaveFragment();
    final Bundle args = new Bundle();
    args.putLongArray(IDS_KEY, ids);
    res.setArguments(args);
    return res;
  }

  @Override
  public void onAttach(Context ctx) {
    super.onAttach(ctx);
    client = ProviderClient.create(ctx);
    usedNames = client.getSavedPlaylists(null).keySet();
  }

  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
    final Context ctx = getActivity();

    name = createEditText(ctx);
    final AlertDialog result = new AlertDialog.Builder(ctx)
        .setTitle(R.string.save)
        .setView(name)
        .setPositiveButton(R.string.save, (dialog, which) -> {
          final Bundle args = getArguments();
          final long[] ids = args != null ? args.getLongArray(IDS_KEY) : null;
          saveAsync(ctx, client, name.getText().toString(), ids);
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

      @Nullable
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
          okButton.setText(usedNames.contains(content) ? R.string.overwrite :
              R.string.save);
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

  private static void saveAsync(final Context ctx, final ProviderClient client,
                                final String name, @Nullable final long[] ids) {

    Executors.newCachedThreadPool().execute(new Runnable() {

      final Toast toast = Toast.makeText(ctx, R.string.saved, Toast.LENGTH_SHORT);

      @Override
      public void run() {
        try {
          client.savePlaylist(name, ids);
        } catch (Exception error) {
          toast.setText(ctx.getString(R.string.save_failed, error.getMessage()));
          toast.setDuration(Toast.LENGTH_LONG);
        }
        toast.show();
      }
    });
  }
}
