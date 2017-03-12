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

import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import java.util.concurrent.TimeUnit;

import app.zxtune.R;
import app.zxtune.RingtoneService;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Item;

public class RingtoneFragment extends DialogFragment {
  
  private static final String TAG = RingtoneFragment.class.getName();

  private static final TimeStamp PREDEFINED_DURATIONS[] = {
    TimeStamp.createFrom(30, TimeUnit.SECONDS),
    TimeStamp.createFrom(1, TimeUnit.MINUTES),
    TimeStamp.createFrom(2, TimeUnit.MINUTES),
    TimeStamp.createFrom(3, TimeUnit.MINUTES),
  };
  
  static DialogFragment createInstance(Item item) {
    final Bundle args = new Bundle();
    args.putParcelable("url", item.getDataId().getFullLocation());
    final DialogFragment result = new RingtoneFragment();
    result.setArguments(args);
    return result;
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    getDialog().setTitle(R.string.ringtone_create_title);
    
    final View result = inflater.inflate(R.layout.ringtone, container, false);
    
    final ViewGroup buttons = (ViewGroup) result.findViewById(R.id.ringtone_durations);
    fillDurations(inflater, buttons);
    return result;
  }
  
  private void fillDurations(LayoutInflater inflater, ViewGroup container) {

    final Uri uri = (Uri) getArguments().getParcelable("url");

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
