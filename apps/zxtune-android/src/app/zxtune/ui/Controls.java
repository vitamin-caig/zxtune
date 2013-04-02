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

import java.io.Closeable;
import java.io.IOException;

import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import app.zxtune.*;
import app.zxtune.Playback.Item;
import app.zxtune.Playback.Status;
import app.zxtune.rpc.BroadcastPlaybackCallbackReceiver;

public class Controls extends Fragment {
  
  private Playback.Control control;
  
  public void setControl(Playback.Control control) {
    this.control = control;
    getView().setEnabled(control != null);
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
}
