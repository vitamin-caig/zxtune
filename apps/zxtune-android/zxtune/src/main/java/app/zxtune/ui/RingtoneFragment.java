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
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import app.zxtune.Permission;
import app.zxtune.R;
import app.zxtune.RingtoneService;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Item;

import java.util.concurrent.TimeUnit;

public class RingtoneFragment extends DialogFragment {
  
  private static final String TAG = RingtoneFragment.class.getName();

  @Nullable
  static DialogFragment createInstance(Activity activity, Item item) {
    if (!Permission.requestSystemSettings(activity)) {
      return null;
    }
    final Bundle args = new Bundle();
    args.putParcelable("url", item.getDataId().getFullLocation());
    final DialogFragment result = new RingtoneFragment();
    result.setArguments(args);
    return result;
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();

    final View view = ((Activity) ctx).getLayoutInflater().inflate(R.layout.ringtone, null, false);

    final ViewGroup buttons = (ViewGroup) view.findViewById(R.id.ringtone_durations);
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
