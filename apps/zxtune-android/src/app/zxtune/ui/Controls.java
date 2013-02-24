/*
 * @file
 * 
 * @brief Controls ui class
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
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import app.zxtune.*;

public class Controls extends Fragment {
  
  private final Playback.Control control;
  
  public Controls(Playback.Control control) {
    this.control = control;
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.controls, null);
  }
  
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    
    view.findViewById(R.id.controls_play).setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.play();
      }
    });
    view.findViewById(R.id.controls_pause).setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.pause();
      }
    });
    view.findViewById(R.id.controls_stop).setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.stop();
      }
    });
  }
  
  public void onClickPlay(View v) {
    control.play();
  }
  
  public void onClickPause(View v) {
    control.pause();
  }
  
  public void onClickStop(View v) {
    control.stop();
  }
}
