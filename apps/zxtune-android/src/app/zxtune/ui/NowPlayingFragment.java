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
import android.view.ViewGroup;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.VisualizerStub;

public class NowPlayingFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private final static String TAG = NowPlayingFragment.class.getName();
  private PlaybackService service;
  private Callback callback;
  private Releaseable callbackConnection;
  private SeekControlView seek;
  private VisualizerView visualizer;
  private InformationView info;
  private PlaybackControlsView ctrls;

  public static Fragment createInstance() {
    return new NowPlayingFragment();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.now_playing, container, false) : null;
  }

  @Override
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    seek = new SeekControlView(view);
    visualizer = (VisualizerView) view.findViewById(R.id.visualizer);
    info = new InformationView(view);
    ctrls = new PlaybackControlsView(view);
    bindViewsToConnectedService();
  }

  @Override
  public synchronized void onDestroy() {
    super.onDestroy();

    unbindFromService();
  }

  @Override
  public synchronized void onServiceConnected(PlaybackService service) {
    this.service = service;
    bindViewsToConnectedService();
  }
  
  private void bindViewsToConnectedService() {
    final boolean serviceConnected = service != null;
    final boolean viewsCreated = visualizer != null;
    if (serviceConnected && viewsCreated) {
      Log.d(TAG, "Subscribe to service events");
      seek.setControl(service.getSeekControl());
      ctrls.setControls(service.getPlaybackControl());
      callback = new PlaybackEvents();
      callbackConnection = new CallbackSubscription(service, new UiThreadCallbackAdapter(getActivity(), callback));
    }
  }
  
  private void unbindFromService() {
    try {
      if (callbackConnection != null) {
        Log.d(TAG, "Unsubscribe from service events");
        callbackConnection.release();
      }
    } finally {
      callbackConnection = null;
      callback.onStatusChanged(false);
    }
    service = null;
  }
  
  private class PlaybackEvents implements Callback {

    @Override
    public void onStatusChanged(boolean isPlaying) {
      visualizer.setSource(isPlaying ? service.getVisualizer() : VisualizerStub.instance());
      seek.setEnabled(isPlaying);
      ctrls.updateStatus(isPlaying);
    }

    @Override
    public void onItemChanged(Item item) {
      info.update(item);
    }
  }
}
