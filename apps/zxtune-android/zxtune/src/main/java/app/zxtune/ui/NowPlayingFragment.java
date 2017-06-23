/**
 *
 * @file
 *
 * @brief Now playing fragment component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.DialogFragment;
import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.support.annotation.Nullable;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import java.io.IOException;

import app.zxtune.Analytics;
import app.zxtune.Log;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.Util;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;
import app.zxtune.playback.PlaybackControlStub;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.PlaybackServiceStub;
import app.zxtune.playback.SeekControlStub;
import app.zxtune.playback.VisualizerStub;

public class NowPlayingFragment extends Fragment implements PlaybackServiceConnection.Callback {

  private static final String TAG = NowPlayingFragment.class.getName();
  private static final int REQUEST_SHARE = 1;
  private static final int REQUEST_SEND = 2;
  private static final String EXTRA_ITEM = TAG + ".EXTRA_LOCATION";

  private PlaybackService service;
  private Callback callback;
  private Releaseable callbackConnection;
  private SeekControlView seek;
  private VisualizerView visualizer;
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
    visualizer = (VisualizerView) view.findViewById(R.id.visualizer);
    info = new InformationView(view);
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
      data.removeExtra(EXTRA_ITEM);
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
      final Item item = data.getParcelableExtra(EXTRA_ITEM);
      Analytics.sendSocialEvent(method, appName, item);

      data.removeExtra(EXTRA_ITEM);
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
    visualizer.setEnabled(false);
    seek.setEnabled(false);
    super.onDestroyView();
  }

  @Override
  public synchronized void onServiceConnected(PlaybackService service) {
    this.service = service;
    Log.d(TAG, "Service connected");
    bindViewsToConnectedService();
  }
  
  private synchronized PlaybackService getService() {
    return this.service;
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
        callback.onStatusChanged(false);
      }
    }
    visualizer.setSource(VisualizerStub.instance());
    seek.setControl(SeekControlStub.instance());
    ctrls.setControls(PlaybackControlStub.instance());
  }
  
  //executed in UI thread only via wrapper
  private class PlaybackEvents implements Callback {
    
    @Override
    public void onStatusChanged(boolean isPlaying) {
      visualizer.setEnabled(isPlaying);
      seek.setEnabled(isPlaying);
      ctrls.updateStatus(isPlaying);
    }

    @Override
    public void onItemChanged(Item item) {
      info.update(item);
      trackActionsMenu.itemChanged(item);
    }
    
    @Override
    public void onIOStatusChanged(boolean isActive) {
      final AppCompatActivity activity = (AppCompatActivity)getActivity();
      //Seems like may be called before activity attach
      if (activity != null) {
        activity.setSupportProgressBarIndeterminateVisibility(isActive);
      }
    }
    
    @Override
    public void onError(final String error) {
      final AppCompatActivity activity = (AppCompatActivity)getActivity();
      //Seems like may be called before activity attach
      if (activity != null) {
        Toast.makeText(activity, error, Toast.LENGTH_SHORT).show();
      }
    }
  }
  
  private class TrackActionsMenu {
    
    private final MenuItem subMenu;
    private final MenuItem add;
    private final MenuItem share;
    private TrackActionsData data;
    
    TrackActionsMenu(Menu menu) {
      this.subMenu = menu.findItem(R.id.action_track);
      this.add = menu.findItem(R.id.action_add);
      this.share = menu.findItem(R.id.action_share);
      subMenu.setEnabled(false);
    }
    
    final boolean selectItem(MenuItem item) throws Exception {
      if (item == null) {
        return false;
      }
      switch (item.getItemId()) {
        case R.id.action_track:
          setupMenu();
          return false;
        case R.id.action_add:
          final Uri location = data.getItem().getDataId().getFullLocation();
          getService().getPlaylistControl().add(new Uri[] {location});
          break;
        case R.id.action_send:
          final Intent toSend = data.makeSendIntent();
          pickAndSend(toSend, item.getTitle(), REQUEST_SEND);
          break;
        case R.id.action_share:
          final Intent toShare = data.makeShareIntent();
          pickAndSend(toShare, item.getTitle(), REQUEST_SHARE);
          break;
        case R.id.action_make_ringtone:
          final DialogFragment fragment = RingtoneFragment.createInstance(data.getItem());
          fragment.show(getActivity().getFragmentManager(), "ringtone");
          break;
        default:
          return false;
      }
      return true;
    }

    private void setupMenu() {
      final Item nowPlaying = getService().getNowPlaying();
      data = new TrackActionsData(getActivity(), nowPlaying);
      add.setEnabled(!data.isFromPlaylist());
      share.setEnabled(data.hasRemotePage());
    }
    
    final void close() {
      data = null;
    }
    
    final void itemChanged(Item item) {
      subMenu.setEnabled(item != null && item != ItemStub.instance());
    }
  }
  
  private static class TrackActionsData {
    
    private final Context context;
    private final Item item;
    
    TrackActionsData(Context context, Item item) {
      this.context = context;
      this.item = item;
    }
    
    final boolean isFromPlaylist() {
      return 0 != item.getId().compareTo(item.getDataId().getFullLocation());
    }
    
    final boolean hasRemotePage() {
      try {
        return null != getRemotePage();
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to get remote page");
        return false;
      }
    }
    
    final Item getItem() {
      return item;
    }
    
    final Intent makeSendIntent() throws Exception {
      final Uri localFile = getLocalPath();
      final Intent result = makeIntent("application/octet");
      result.putExtra(Intent.EXTRA_SUBJECT, getTitle());
      result.putExtra(Intent.EXTRA_TEXT, getSendText());
      result.putExtra(Intent.EXTRA_STREAM, localFile);
      result.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
      return result;
    }
    
    final Intent makeShareIntent() throws Exception {
      //text/html is not recognized by twitter/facebook
      final Intent result = makeIntent("text/plain");
      result.putExtra(Intent.EXTRA_SUBJECT, getTitle());
      result.putExtra(Intent.EXTRA_TEXT, getShareText());
      return result;
    }
    
    private Intent makeIntent(String mime) {
      final Intent result = new Intent(Intent.ACTION_SEND);
      result.setType(mime);
      result.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
      result.putExtra(EXTRA_ITEM, (Parcelable) item);
      return result;
    }
    
    private String getTitle() throws Exception {
      return Util.formatTrackTitle(item.getTitle(), item.getAuthor(), item.getDataId().getDisplayFilename());
    }
    
    private String getSendText() throws IOException {
      final String remotePage = getRemotePage();
      return context.getString(R.string.send_text, remotePage != null ? remotePage : "");
    }
    
    private String getShareText() throws IOException {
      return context.getString(R.string.share_text, getRemotePage());
    }
    
    private Uri getLocalPath() throws IOException {
      final Uri uri = item.getDataId().getDataLocation();
      if (uri.getScheme().equals("file")) {
        return uri;
      }
      final VfsFile file = openFile(uri);
      return createLocalPath(file);
    }

    @Nullable
    private String getRemotePage() throws IOException {
      final VfsFile file = openFile(item.getDataId().getDataLocation());
      if (file != null) {
        return (String) file.getExtension(VfsExtensions.SHARE_URL);
      } else {
        return null;
      }
    }
    
    private static VfsFile openFile(Uri uri) throws IOException {
      final VfsObject obj = Vfs.resolve(uri);
      if (obj instanceof VfsFile) {
        return (VfsFile) obj;
      } else {
        throw new IOException("Failed to open " + uri);
      }
    }
    
    private Uri createLocalPath(VfsFile file) throws IOException {
      //TODO: rework
      final VfsCache cache = VfsCache.create(context).createNested("sent");
      final String filename = file.getUri().getLastPathSegment();
      return cache.putAnyCachedFileContent(filename, file.getContent());
    }
  }
}
