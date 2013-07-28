/*
 * @file
 * @brief NowPlaying fragment class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.RetainedCallbackSubscriptionFragment;
import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Iterator;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.SeekControl;
import app.zxtune.playback.Visualizer;

public class NowPlayingFragment extends Fragment {

  private final static String TAG = NowPlayingFragment.class.getName();
  private Releaseable connection;
  private Control control;
  private SeekControlView seek;
  private VisualizerView visualizer;
  private InformationView info;
  private PlaybackControlsView ctrls;

  public static Fragment createInstance() {
    return new NowPlayingFragment();
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
        } else {
          stopUpdating();
        }
        ctrls.updateStatus();
      }

      @Override
      public void onItemChanged(Item item) {
        info.update(item);
      }
    });
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.now_playing, container, false) : null;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    seek = new SeekControlView(view);
    visualizer = (VisualizerView) view.findViewById(R.id.visualizer);
    info = new InformationView(view);
    ctrls = new PlaybackControlsView(view);
    ctrls.setControls(new PlaybackControl() {
      @Override
      public void play() {
        control.play();
      }
      
      @Override
      public void stop() {
        control.stop();
      }
      
      @Override
      public boolean isPlaying() {
        return control.isPlaying();
      }
      
      @Override
      public void prev() {
        control.prev();
      }
      
      @Override
      public void next() {
        control.next();
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
    seek.setControl(new SeekControl() {
      @Override
      public TimeStamp getDuration() {
        return control.getItem().getDuration();
      }

      @Override
      public TimeStamp getPosition() {
        return control.getPlaybackPosition();
      }

      @Override
      public void setPosition(TimeStamp position) {
        control.setPlaybackPosition(position);
      }
    });
    visualizer.setSource(new Visualizer() {
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
    seek.setControl(null);
    visualizer.setSource(null);
  }
}
