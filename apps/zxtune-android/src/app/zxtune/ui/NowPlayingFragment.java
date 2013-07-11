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
import app.zxtune.sound.Visualizer;

public class NowPlayingFragment extends Fragment {

  private final static String TAG = NowPlayingFragment.class.getName();
  private final Handler timer;
  private final Runnable updateUiTask;
  private Releaseable connection;
  private Control control;
  private SpectrumAnalyzerView analyzer;
  private ImageButton prev;
  private ImageButton playPause;
  private ImageButton next;
  private SeekBar seek;
  private TextView time;

  public static Fragment createInstance() {
    return new NowPlayingFragment();
  }

  public NowPlayingFragment() {
    this.timer = new Handler();
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
          onStatusChanged(control.isPlaying());
          onItemChanged(control.getItem());
        } else {
          stopUpdating();
        }
      }

      @Override
      public void onStatusChanged(boolean nowPlaying) {
        if (nowPlaying) {
          startUpdating();
          playPause.setImageResource(R.drawable.ic_pause);
        } else {
          stopUpdating();
          playPause.setImageResource(R.drawable.ic_play);
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
    prev = (ImageButton) view.findViewById(R.id.controls_prev);
    playPause = (ImageButton) view.findViewById(R.id.controls_play_pause);
    next = (ImageButton) view.findViewById(R.id.controls_next);
    seek = (SeekBar) view.findViewById(R.id.position_seek);
    time = (TextView) view.findViewById(R.id.position_time);
    prev.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.prev();
      }
    });
    playPause.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        if (control.isPlaying()) {
          control.stop();
        } else {
          control.play();
        }
      }
    });
    next.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        control.next();
      }
    });
    seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
          final TimeStamp pos = TimeStamp.createFrom(progress, TimeUnit.SECONDS);
          control.setPlaybackPosition(pos);
        }
      }
      
      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {}
      
      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {}
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
    timer.post(updateUiTask);
    analyzer.setSource(new Visualizer() {
      @Override
      public int getSpectrum(int[] bands, int[] levels) {
        final int[] spc = control.getSpectrumAnalysis();
        assert bands.length >= spc.length;
        assert levels.length >= spc.length;
        for (int idx = 0; idx != spc.length; ++idx) {
          final int val = spc[idx];
          bands[idx] = val & 0xff;
          levels[idx] = val >> 8;
        }
        return spc.length;
      }
    });
  }

  private void stopUpdating() {
    Log.d(TAG, "Stop updating UI");
    timer.removeCallbacks(updateUiTask);
    analyzer.setSource(null);
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
