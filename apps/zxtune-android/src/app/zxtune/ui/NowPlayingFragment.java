/*
 * @file
 * @brief NowPlaying fragment class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.concurrent.TimeUnit;

import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.RetainedCallbackSubscriptionFragment;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;

public class NowPlayingFragment extends Fragment {

  private final static String TAG = NowPlayingFragment.class.getName();
  private final Handler timer;
  private final Runnable updateAnalyzerTask;
  private final Runnable updateUiTask;
  private Releaseable connection;
  private Control control;
  private SpectrumAnalyzerView analyzer;
  private ImageButton playPause; 
  private SeekBar seek;
  private TextView time;

  public static Fragment createInstance() {
    return new NowPlayingFragment();
  }

  public NowPlayingFragment() {
    this.timer = new Handler();
    this.updateAnalyzerTask = new UpdateAnalyzerTask();
    this.updateUiTask = new UpdateUiTask();
  }
  
  @Override
  public void onActivityCreated(Bundle savedInstanceState) {
    super.onActivityCreated(savedInstanceState);
    
    assert connection == null;
  
    Log.d(TAG, "Subscribe for service events");
    final CallbackSubscription subscription = RetainedCallbackSubscriptionFragment.find(getFragmentManager());
    connection = subscription.subscribe(new Callback() {
      @Override
      public void onControlChanged(Control control) {
        NowPlayingFragment.this.control = control;
        final boolean connected = control != null;
        final View view = getView();
        if (view != null) {
          view.setEnabled(connected);
        }
        if (connected) {
          onStatusChanged(control.getStatus());
          onItemChanged(control.getItem());
        } else {
          stopUpdating();
        }
      }

      @Override
      public void onStatusChanged(Status status) {
        switch (status) {
          case PLAYING:
            startUpdating();
            playPause.setImageResource(R.drawable.ic_pause);
            break;
          case STOPPED:
            seek.setProgress(0);
            time.setText(R.string.stub_time);
          default:
            stopUpdating();
            playPause.setImageResource(R.drawable.ic_play);
            break;
        }
      }

      @Override
      public void onItemChanged(Item item) {
        if (item != null) {
          seek.setMax((int) item.getDuration().convertTo(TimeUnit.SECONDS));
        }
      }
    });
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.now_playing, container, false) : null;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    analyzer = (SpectrumAnalyzerView) view.findViewById(R.id.analyzer);
    playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    seek = (SeekBar) view.findViewById(R.id.position_seek);
    time = (TextView) view.findViewById(R.id.position_time);
    playPause.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.playPause();
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
  public void onDestroy() {
    super.onDestroy();

    try {
      if (connection != null) {
        Log.d(TAG, "Unsubscribe from service events");
        connection.release();
      }
    } finally {
      connection = null;
    }
  }

  private void startUpdating() {
    Log.d(TAG, "Start updating UI");
    timer.post(updateAnalyzerTask);
    timer.post(updateUiTask);
  }

  private void stopUpdating() {
    Log.d(TAG, "Stop updating UI");
    timer.removeCallbacks(updateUiTask);
    timer.removeCallbacks(updateAnalyzerTask);
  }
  
  private class UpdateAnalyzerTask implements Runnable {
    
    @Override
    public void run() {
      final int[] spectrum = control.getSpectrumAnalysis();
      if (spectrum == null) {
        return;
      }
      analyzer.update(spectrum);
      
      timer.postDelayed(this, 100);
    }
  }

  private class UpdateUiTask implements Runnable {
    
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
}
