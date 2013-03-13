/*
 * @file
 * 
 * @brief Position ui class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SeekBar;
import app.zxtune.*;

public class Position extends Fragment {

  private Playback.Control control;
  private final Playback.Callback callback = new StatusCallback();
  private SeekBar position;

  public void setControl(Playback.Control control) {
    this.control = control;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.position, null);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    position = (SeekBar) view.findViewById(R.id.position_seek);
  }

  @Override
  public void onDestroyView() {
    super.onDestroyView();
    position = null;
  }

  @Override
  public void onStart() {
    super.onStart();
    control.registerCallback(callback);
  }

  @Override
  public void onStop() {
    super.onStop();
    control.unregisterCallback(callback);
  }

  class StatusCallback implements Playback.Callback {
    public void started(String description, int duration) {
      position.setMax(duration);
    }

    public void paused(String description) {

    }

    public void stopped() {

    }

    public void positionChanged(int curFrame, String curTime) {
      if (position != null) {
        position.setProgress(curFrame);
      }
    }
  }
}
