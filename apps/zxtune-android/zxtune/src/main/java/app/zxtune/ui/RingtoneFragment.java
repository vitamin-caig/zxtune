/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;
import androidx.appcompat.app.AlertDialog;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import app.zxtune.Permission;
import app.zxtune.R;
import app.zxtune.RingtoneService;
import app.zxtune.TimeStamp;

import java.util.concurrent.TimeUnit;

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
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();

    final View view = ((Activity) ctx).getLayoutInflater().inflate(R.layout.ringtone, null, false);

    final ViewGroup buttons = view.findViewById(R.id.ringtone_durations);
    fillDurations(buttons);

    final AlertDialog result = new AlertDialog.Builder(ctx)
                                   .setTitle(R.string.ringtone_create_title)
                                   .setView(view)
                                   .create();
    return result;
  }

  private void fillDurations(ViewGroup container) {

    final Uri uri = getArguments().getParcelable("url");
    if (uri == null) {
      return;
    }

    final Button.OnClickListener listener = new Button.OnClickListener() {
      @Override
      public void onClick(View v) {
        final String tag = (String) v.getTag();
        final Integer tagValue = Integer.valueOf(tag);
        RingtoneService.execute(getActivity(), uri, TimeStamp.createFrom(tagValue, TimeUnit.SECONDS));
        dismiss();
      }
    };
    for (int idx = 0, lim = container.getChildCount(); idx != lim; ++idx) {
      final Button button = (Button) container.getChildAt(idx);
      button.setOnClickListener(listener);
    }
  }
}
