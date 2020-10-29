/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.app.Activity;
import android.app.Dialog;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;

import java.util.concurrent.TimeUnit;

import app.zxtune.Permission;
import app.zxtune.R;
import app.zxtune.RingtoneService;
import app.zxtune.TimeStamp;

public class RingtoneFragment extends DialogFragment {

  @Nullable
  static DialogFragment createInstance(FragmentActivity activity, Uri location) {
    if (!Permission.requestSystemSettings(activity)) {
      return null;
    }
    final Bundle args = new Bundle();
    args.putParcelable("url", location);
    final DialogFragment result = new RingtoneFragment();
    result.setArguments(args);
    return result;
  }

  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
    final Activity ctx = getActivity();

    final View view = ctx.getLayoutInflater().inflate(R.layout.ringtone, null, false);

    final ViewGroup buttons = view.findViewById(R.id.ringtone_durations);
    fillDurations(buttons);

    return new AlertDialog.Builder(ctx)
                                   .setTitle(R.string.ringtone_create_title)
                                   .setView(view)
                                   .create();
  }

  private void fillDurations(ViewGroup container) {

    final Uri uri = getArguments().getParcelable("url");
    if (uri == null) {
      return;
    }

    final Button.OnClickListener listener = v -> {
      final String tag = (String) v.getTag();
      final int tagValue = Integer.parseInt(tag);
      RingtoneService.execute(getActivity(), uri, TimeStamp.createFrom(tagValue, TimeUnit.SECONDS));
      dismiss();
    };
    for (int idx = 0, lim = container.getChildCount(); idx != lim; ++idx) {
      final Button button = (Button) container.getChildAt(idx);
      button.setOnClickListener(listener);
    }
  }
}
