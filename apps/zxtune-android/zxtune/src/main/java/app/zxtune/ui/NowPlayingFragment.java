/**
 * @file
 * @brief Now playing fragment component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.MainService;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.stubs.PlaybackControlStub;
import app.zxtune.playback.stubs.PlaybackServiceStub;
import app.zxtune.playback.stubs.SeekControlStub;
import app.zxtune.playback.stubs.VisualizerStub;
import app.zxtune.ui.controllers.VisualizerController;
import app.zxtune.ui.utils.UiThreadCallbackAdapter;
import app.zxtune.ui.views.SpectrumAnalyzerView;

public class NowPlayingFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = NowPlayingFragment.class.getName();
  private static final int REQUEST_SHARE = 1;
  private static final int REQUEST_SEND = 2;
  private static final String EXTRA_ITEM_LOCATION = TAG + ".EXTRA_LOCATION";

  private PlaybackService service;
  private Callback callback;
  private Releaseable callbackConnection;
  private final VisualizerController visualizer = new VisualizerController();
  private SeekControlView seek;
  private InformationView info;
  private PlaybackControlsView ctrls;
  private TrackActionsMenu trackActionsMenu;

  public static Fragment createInstance() {
    return new NowPlayingFragment();
  }

  public NowPlayingFragment() {
    this.service = PlaybackServiceStub.instance();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setHasOptionsMenu(true);
  }

  @Override
  public synchronized void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
    super.onCreateOptionsMenu(menu, inflater);

    inflater.inflate(R.menu.track, menu);

    trackActionsMenu = new TrackActionsMenu(menu);

    // onOptionsMenuClosed is not called, see multiple bugreports e.g.
    // https://code.google.com/p/android/issues/detail?id=2410
    // https://code.google.com/p/android/issues/detail?id=2746
    // https://code.google.com/p/android/issues/detail?id=176377
    final ActionBar bar = ((AppCompatActivity) getActivity()).getSupportActionBar();
    if (bar != null) {
      bar.addOnMenuVisibilityListener(new ActionBar.OnMenuVisibilityListener() {
        @Override
        public void onMenuVisibilityChanged(boolean isVisible) {
          if (!isVisible) {
            trackActionsMenu.close();
          }
        }
      });
    }
    bindViewsToConnectedService();
  }

  /*
   * Assume that menu is shown quite rarely. So keep current track state while menu is visible
   */

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    try {
      return trackActionsMenu.selectItem(item)
                 || super.onOptionsItemSelected(item);
    } catch (Exception e) {//use the most common type
      Log.w(TAG, e, "onOptionsItemSelected");
      final Throwable cause = e.getCause();
      final String msg = cause != null ? cause.getMessage() : e.getMessage();
      Toast.makeText(getActivity(), msg, Toast.LENGTH_SHORT).show();
    }
    return true;
  }

  @Override
  @Nullable
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.now_playing, container, false) : null;
  }

  @Override
  public synchronized void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    seek = new SeekControlView(view);
    visualizer.setView((SpectrumAnalyzerView) view.findViewById(R.id.visualizer));
    info = new InformationView(getActivity(), view);
    ctrls = new PlaybackControlsView(view);
    bindViewsToConnectedService();
  }

  private void pickAndSend(Intent data, CharSequence title, int code) {
    try {
      final Intent picker = new Intent(Intent.ACTION_PICK_ACTIVITY);
      picker.putExtra(Intent.EXTRA_TITLE, title);
      picker.putExtra(Intent.EXTRA_INTENT, data);
      startActivityForResult(picker, code);
    } catch (SecurityException e) {
      //workaround for Huawei requirement for huawei.android.permission.HW_SIGNATURE_OR_SYSTEM
      data.removeExtra(EXTRA_ITEM_LOCATION);
      final Intent chooser = Intent.createChooser(data, title);
      startActivity(chooser);
    }
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data) {
    final boolean isShare = requestCode == REQUEST_SHARE;
    final boolean isSend = requestCode == REQUEST_SEND;
    if (data != null && (isShare || isSend)) {
      final String method = isShare ? "Share" : "Send";
      final String appName = data.getComponent().getPackageName();
      final Uri location = data.getParcelableExtra(EXTRA_ITEM_LOCATION);
      Analytics.sendSocialEvent(method, appName, location);

      data.removeExtra(EXTRA_ITEM_LOCATION);
      startActivity(data);
    }
  }

  @Override
  public synchronized void onStart() {
    super.onStart();
    bindViewsToConnectedService();
  }

  @Override
  public synchronized void onStop() {
    super.onStop();
    unbindFromService();
  }

  @Override
  public void onDestroyView() {
    visualizer.shutdown();
    seek.setEnabled(false);
    super.onDestroyView();
  }

  @Override
  public synchronized void onServiceConnected(PlaybackService service) {
    this.service = service;
    Log.d(TAG, "Service connected");
    bindViewsToConnectedService();
  }

  // relative order of onViewCreated/onCreateOptionsMenu/onServiceConnected is not specified
  private void bindViewsToConnectedService() {
    assert service != null;
    final boolean serviceConnected = service != PlaybackServiceStub.instance();
    final boolean viewsCreated = visualizer != null;
    final boolean menuCreated = trackActionsMenu != null;
    if (serviceConnected && viewsCreated && menuCreated) {
      Log.d(TAG, "Subscribe to service events");
      visualizer.setSource(service.getVisualizer());
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
      //TODO: rework synchronization scheme
      if (callback != null) {
        callback.onStateChanged(PlaybackControl.State.STOPPED);
      }
    }
    visualizer.setSource(VisualizerStub.instance());
    seek.setControl(SeekControlStub.instance());
    ctrls.setControls(PlaybackControlStub.instance());
  }

  //executed in UI thread only via wrapper
  private class PlaybackEvents implements Callback {

    @Override
    public void onInitialState(PlaybackControl.State state, Item item) {
      onStateChanged(state);
      onItemChanged(item);
    }

    @Override
    public void onStateChanged(PlaybackControl.State state) {
      final boolean isPlaying = state == PlaybackControl.State.PLAYING;
      if (isPlaying) {
        visualizer.startUpdates();
      } else {
        visualizer.stopUpdates();
      }
      seek.setEnabled(isPlaying);
      ctrls.updateStatus(isPlaying);
    }

    @Override
    public void onItemChanged(Item item) {
      seek.update();
    }

    @Override
    public void onError(final String error) {
      final AppCompatActivity activity = (AppCompatActivity) getActivity();
      //Seems like may be called before activity attach
      if (activity != null) {
        Toast.makeText(activity, error, Toast.LENGTH_SHORT).show();
      }
    }
  }

  private class TrackActionsMenu {

    private final MenuItem add;
    private final MenuItem share;
    private TrackActionsData data;

    TrackActionsMenu(Menu menu) {
      this.add = menu.findItem(R.id.action_add);
      this.share = menu.findItem(R.id.action_share);
    }

    final boolean selectItem(MenuItem item) {
      if (item == null) {
        return false;
      }
      switch (item.getItemId()) {
        case R.id.action_track:
          setupMenu();
          return false;
        case R.id.action_add:
          addCurrent();
          break;
          /* TODO: rework
        case R.id.action_send:
          final Intent toSend = data.makeSendIntent();
          pickAndSend(toSend, item.getTitle(), REQUEST_SEND);
          break;
          */
        case R.id.action_share:
          final Intent toShare = data.makeShareIntent();
          pickAndSend(toShare, item.getTitle(), REQUEST_SHARE);
          break;
        case R.id.action_make_ringtone:
          final DialogFragment fragment = RingtoneFragment.createInstance(getActivity(), data.getFullLocation());
          if (fragment != null) {
            fragment.show(getActivity().getSupportFragmentManager(), "ringtone");
          }
          break;
        default:
          return false;
      }
      return true;
    }

    private void setupMenu() {
      final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(getActivity());
      final MediaMetadataCompat metadata = ctrl != null ? ctrl.getMetadata() : null;
      if (ctrl != null) {
        data = new TrackActionsData(getActivity(), metadata);
        add.setEnabled(!data.isFromPlaylist());
        share.setEnabled(data.hasRemotePage());
      } else {
        data = null;
        add.setEnabled(false);
        share.setEnabled(false);
      }
    }

    private void addCurrent() {
      final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(getActivity());
      if (ctrl != null) {
        ctrl.getTransportControls().sendCustomAction(MainService.CUSTOM_ACTION_ADD_CURRENT, null);
      }
    }

    final void close() {
      data = null;
    }
  }

  private static class TrackActionsData {

    private final Context context;
    private final MediaMetadataCompat metadata;
    private final MediaDescriptionCompat description;

    TrackActionsData(Context context, @NonNull MediaMetadataCompat metadata) {
      this.context = context;
      this.metadata = metadata;
      this.description = metadata.getDescription();
    }

    final boolean isFromPlaylist() {
      return !description.getMediaUri().toString().equals(description.getMediaId());
    }

    final boolean hasRemotePage() {
      return null != getRemotePage();
    }

    final Uri getFullLocation() {
      return description.getMediaUri();
    }

    final Intent makeShareIntent() {
      //text/html is not recognized by twitter/facebook
      final Intent result = makeIntent("text/plain");
      result.putExtra(Intent.EXTRA_SUBJECT, description.getTitle());
      result.putExtra(Intent.EXTRA_TEXT, getShareText());
      return result;
    }

    private Intent makeIntent(String mime) {
      final Intent result = new Intent(Intent.ACTION_SEND);
      result.setType(mime);
      result.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
      result.putExtra(EXTRA_ITEM_LOCATION, description.getMediaUri());
      return result;
    }

    private String getShareText() {
      return context.getString(R.string.share_text, getRemotePage());
    }

    @Nullable
    private String getRemotePage() {
      return metadata.getString(VfsExtensions.SHARE_URL);
    }
  }
}
