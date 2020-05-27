/**
 *
 * @file
 *
 * @brief SeekBarPreference component
 *
 * @author vitamin.caig@gmail.com
 * @author Originally taken from http://robobunny.com/wp/2013/08/24/android-seekbar-preference-v2/
 *
 */

package app.zxtune.ui.utils;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import androidx.annotation.Nullable;

import app.zxtune.Log;
import app.zxtune.R;

public class SeekBarPreference extends Preference implements OnSeekBarChangeListener {

  private static final String TAG = SeekBarPreference.class.getName();

  private static final String ANDROIDNS = "http://schemas.android.com/apk/res/android";

  private static final int DEFAULT_VALUE = 50;

  private int maxValue = 100;
  private int currentValue;
  @Nullable
  private SeekBar seekBar;
  @Nullable
  private TextView statusText;

  private SeekBarPreference(Context context, AttributeSet attrs) {
    super(context, attrs);
    initPreference(context, attrs);
  }

  private SeekBarPreference(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    initPreference(context, attrs);
  }

  private void initPreference(Context context, AttributeSet attrs) {
    setValuesFromXml(attrs);
    seekBar = new SeekBar(context, attrs);
    seekBar.setMax(maxValue);
    seekBar.setOnSeekBarChangeListener(this);

    setWidgetLayoutResource(R.layout.seek_bar_preference);
  }

  private void setValuesFromXml(AttributeSet attrs) {
    maxValue = attrs.getAttributeIntValue(ANDROIDNS, "max", 100);
  }

  @Override
  protected View onCreateView(ViewGroup parent) {
    final View view = super.onCreateView(parent);
    // The basic preference layout puts the widget frame to the right of the title and summary,
    // so we need to change it a bit - the seekbar should be under them.
    final LinearLayout layout = (LinearLayout) view;
    layout.setOrientation(LinearLayout.VERTICAL);
  
    return view;
  }
  
  @Override
  public void onBindView(View view) {
    super.onBindView(view);

    try {
      // move our seekbar to the new view we've been given
      final ViewParent oldContainer = seekBar.getParent();
      final ViewGroup newContainer = view.findViewById(R.id.seekBarPrefBarContainer);

      if (oldContainer != newContainer) {
        // remove the seekbar from the old view
        if (oldContainer != null) {
          ((ViewGroup) oldContainer).removeView(seekBar);
        }
        // remove the existing seekbar (there may not be one) and add ours
        newContainer.removeAllViews();
        newContainer.addView(seekBar, ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Error binding view");
    }

    //if dependency is false from the beginning, disable the seek bar
    if (view != null && !view.isEnabled()) {
      seekBar.setEnabled(false);
    }
    if (view != null) {
      updateView(view);
    }
  }

  /**
   * Update a SeekBarPreference view with our current state
   * 
   * @param view view container to update
   */
  private void updateView(View view) {

    try {
      statusText = view.findViewById(R.id.seekBarPrefValue);

      statusText.setText(String.valueOf(currentValue));

      seekBar.setProgress(currentValue);
    } catch (Exception e) {
      Log.w(TAG, e, "Error updating seek bar preference");
    }
  }

  @Override
  public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
    final int newValue = Math.min(progress, maxValue);

    // change rejected, revert to the previous value
    if (!callChangeListener(newValue)) {
      seekBar.setProgress(currentValue);
      return;
    }

    // change accepted, store it
    currentValue = newValue;
    statusText.setText(String.valueOf(newValue));
    persistInt(newValue);
  }

  @Override
  public void onStartTrackingTouch(SeekBar seekBar) {}

  @Override
  public void onStopTrackingTouch(SeekBar seekBar) {
    notifyChanged();
  }


  @Override
  protected Object onGetDefaultValue(TypedArray ta, int index) {
    return ta.getInt(index, DEFAULT_VALUE);
  }

  @Override
  protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {

    if (restoreValue) {
      currentValue = getPersistedInt(currentValue);
    } else {
      int temp = 0;
      try {
        temp = (Integer) defaultValue;
      } catch (Exception e) {
        Log.w(TAG, e, "Invalid default value: %s", defaultValue);
      }

      persistInt(temp);
      currentValue = temp;
    }
  }

  /**
   * make sure that the seekbar is disabled if the preference is disabled
   */
  @Override
  public void setEnabled(boolean enabled) {
    super.setEnabled(enabled);
    seekBar.setEnabled(enabled);
  }

  @Override
  public void onDependencyChanged(Preference dependency, boolean disableDependent) {
    super.onDependencyChanged(dependency, disableDependent);

    //Disable movement of seek bar when dependency is false
    if (seekBar != null) {
      seekBar.setEnabled(!disableDependent);
    }
  }
}
