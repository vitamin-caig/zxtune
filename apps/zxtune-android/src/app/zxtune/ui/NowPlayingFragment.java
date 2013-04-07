/*
 * @file
 * 
 * @brief NowPlaying fragment class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.io.Closeable;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.SeekBar;
import android.widget.TextView;
import app.zxtune.Playback;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.rpc.BroadcastPlaybackCallbackReceiver;

public class NowPlayingFragment extends Fragment implements Playback.Callback {

  private Playback.Control control;
  private SeekBar seek;
  private TextView time;
  private final Handler timer = new Handler();
  private final Runnable timerTask = new Runnable() {
    @Override
    public void run() {
      final TimeStamp pos = control.getPlaybackPosition();
      if (pos == null) {
        return;
      }
      seek.setProgress((int) pos.convertTo(TimeUnit.SECONDS));
      time.setText(pos.toString());

      timer.postDelayed(this, 1000);
    }
  };
  private Closeable callbackHandler;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return inflater.inflate(R.layout.now_playing, null);
  }
  
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    seek = (SeekBar) view.findViewById(R.id.position_seek);
    time = (TextView) view.findViewById(R.id.position_time);
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
  
  @Override
  public void onStart() {
    super.onStart();
    callbackHandler = new BroadcastPlaybackCallbackReceiver(getActivity(), this);
  }

  @Override
  public void onStop() {
    super.onStop();
    try {
      callbackHandler.close();
    } catch (IOException e) {
    } finally {
      callbackHandler = null;
    }
  }
  
  public void setControl(Playback.Control control) {
    this.control = control;
    final boolean connected = control != null;
    getView().setEnabled(connected);
    if (connected) {
      itemChanged(control.getItem());
      statusChanged(control.getStatus());
    } else {
      stopUpdating();
    }
  }

  @Override
  public void itemChanged(Playback.Item item) {
    if (item != null) {
      seek.setMax((int) item.getDuration().convertTo(TimeUnit.SECONDS));
    }
  }

  @Override
  public void statusChanged(Playback.Status status) {
    switch (status) {
      case PLAYING:
        startUpdating();
        break;
      case STOPPED:
        seek.setProgress(0);
        time.setText(R.string.stub_time);
      default:
        stopUpdating();
    }
  }
  
  private void startUpdating() {
    timer.post(timerTask);
  }
  
  private void stopUpdating() {
    timer.removeCallbacks(timerTask);
  }
}
