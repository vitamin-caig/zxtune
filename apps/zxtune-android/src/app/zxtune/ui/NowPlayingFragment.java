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

import java.io.File;
import java.io.IOException;
import java.util.Locale;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import app.zxtune.PlaybackServiceConnection;
import app.zxtune.R;
import app.zxtune.Releaseable;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;
import app.zxtune.playback.PlaybackService;
import app.zxtune.playback.PlaybackServiceStub;
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
  private Actions actions;

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
  public void onCreateOptionsMenu (Menu menu, MenuInflater inflater) {
    super.onCreateOptionsMenu(menu, inflater);

    inflater.inflate(R.menu.track, menu);
    actions = new Actions(menu);
    bindViewsToConnectedService();
  }
  
  @Override
  public boolean onOptionsItemSelected (MenuItem menu) {
    switch (menu.getItemId()) {
      case R.id.action_add:
        service.getPlaylistControl().add(new Uri[] {service.getNowPlaying().getDataId()});
        //disable further addings
        menu.setVisible(false);
        break;
      case R.id.action_send:
        final Intent toSend = Actions.makeSendIntent(new ShareData(getActivity(), service.getNowPlaying()));
        if (toSend != null) {
          startActivity(Intent.createChooser(toSend, menu.getTitle()));
        }
        break;
      case R.id.action_share:
        final Intent toShare = Actions.makeShareIntent(new ShareData(getActivity(), service.getNowPlaying()));
        if (toShare != null) {
          startActivity(Intent.createChooser(toShare, menu.getTitle()));
        }
        break;
      default:
        return super.onOptionsItemSelected(menu);
    }
    return true;
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
  
  // relative order of onViewCreated/onCreateOptionsMenu/onServiceConnected is not specified
  private void bindViewsToConnectedService() {
    assert service != null;
    final boolean serviceConnected = service != PlaybackServiceStub.instance();
    final boolean viewsCreated = visualizer != null;
    final boolean menuCreated = actions != null;
    if (serviceConnected && viewsCreated && menuCreated) {
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
      //TODO: rework synchronization scheme
      if (callback != null) {
        callback.onStatusChanged(false);
      }
    }
    service = PlaybackServiceStub.instance();
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
      actions.update(item);
    }
    
    @Override
    public void onIOStatusChanged(boolean isActive) {
      final ActionBarActivity activity = (ActionBarActivity)getActivity();
      //Seems like may be called before activity attach
      if (activity != null) {
        activity.setSupportProgressBarIndeterminateVisibility(isActive);
      }
    }
  }
  
  private static class Actions {

    private final MenuItem subMenu;
    private final MenuItem add;
    private final MenuItem share;
    
    Actions(Menu menu) {
      this.subMenu = menu.findItem(R.id.action_track);
      this.add = menu.findItem(R.id.action_add);
      this.share = menu.findItem(R.id.action_share);
      subMenu.setEnabled(false);
    }
    
    final void update(Item item) {
      if (item != null && item != ItemStub.instance()) {
        subMenu.setEnabled(true);
        updateAddStatus(item);
        updateShareStatus(item);
      } else {
        subMenu.setEnabled(false);
      }
    }
    
    private void updateAddStatus(Item item) {
      final boolean isVfsItem = item.getId().equals(item.getDataId());
      add.setVisible(isVfsItem);
    }
    
    private void updateShareStatus(Item item) {
      final boolean hasRemotePage = ShareData.hasRemotePage(item.getDataId()); 
      share.setEnabled(hasRemotePage);
    }
    
    static Intent makeSendIntent(ShareData data) {
      final Uri localFile = data.getLocalPath();
      if (localFile == null) {
        return null;
      }
      final Intent result = makeIntent("application/octet");
      result.putExtra(Intent.EXTRA_SUBJECT, data.getTitle());
      result.putExtra(Intent.EXTRA_TEXT, data.getSendText());
      result.putExtra(Intent.EXTRA_STREAM, localFile);
      result.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
      return result;
    }
    
    static Intent makeShareIntent(ShareData data) {
      //text/html is not recognized by twitter/facebook
      final Intent result = makeIntent("text/plain");
      result.putExtra(Intent.EXTRA_SUBJECT, data.getTitle());
      result.putExtra(Intent.EXTRA_TEXT, data.getShareText());
      return result;
    }
    
    private static Intent makeIntent(String mime) {
      final Intent result = new Intent(Intent.ACTION_SEND);
      result.setType(mime);
      result.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
      return result;
    }
  }
  
  private static class ShareData {
    
    private final Context context;
    private final Item item;
    
    private static final RemoteCatalog CATALOGS[] = {
      new ZXTunesRemoteCatalog(),
      new ZXArtRemoteCatalog()
    };
    
    ShareData(Context context, Item item) {
      this.context = context;
      this.item = item;
    }
    
    public final String getTitle() {
      final String title = item.getTitle();
      final String author = item.getAuthor();
      final boolean noTitle = 0 == title.length();
      final boolean noAuthor = 0 == author.length();
      final StringBuilder result = new StringBuilder();
      if (noTitle && noAuthor) {
        final String filename = item.getDataId().getLastPathSegment();
        result.append(filename);
      } else {
        result.append(title);
        if (!noTitle && !noAuthor) {
          result.append(" - ");
        }
        result.append(author);
      }
      return result.toString();
    }
    
    public final String getSendText() {
      final String page = getRemotePage();
      return context.getString(R.string.send_text, page != null ? page : "");
    }
    
    public final String getShareText() {
      return context.getString(R.string.share_text, getRemotePage());
    }
    
    public final Uri getLocalPath() {
      final Uri uri = item.getDataId();
      if (uri.getScheme().equals("file")) {
        return uri;
      }
      final VfsFile file = openFile(uri);
      return createLocalPath(file);
    }

    public final String getRemotePage() {
      final Uri uri = item.getDataId();
      final String scheme = uri.getScheme();
      for (RemoteCatalog cat : CATALOGS) {
        if (cat.checkScheme(scheme)) {
          return cat.getPage(uri);
        }
      }
      return null;
    }
    
    private VfsFile openFile(Uri uri) {
      try {
        final VfsRoot root = Vfs.createRoot(context);
        final VfsObject obj = root.resolve(uri);
        return obj instanceof VfsFile
          ? (VfsFile) obj
          : null;
      } catch (IOException e) {
        Log.d(TAG, "Failed to open " + uri, e);
        return null;
      }
    }
    
    private Uri createLocalPath(VfsFile file) {
      try {
        final VfsCache cache = VfsCache.createExternal(context, "sent");
        final String filename = file.getUri().getLastPathSegment();
        cache.putAnyCachedFileContent(filename, file.getContent());
        final File local = cache.getCachedFile(filename);
        return Uri.fromFile(local); 
      } catch (IOException e) {
        Log.d(TAG, "Failed to create local file copy", e);
        return null;
      }
    }
    
    static boolean hasRemotePage(Uri uri) {
      final String scheme = uri.getScheme();
      for (RemoteCatalog cat : CATALOGS) {
        if (cat.checkScheme(scheme)) {
          return true;
        }
      }
      return false;
    }
    
    //TODO: integrate to VFS
    private interface RemoteCatalog {
      boolean checkScheme(String scheme);
      String getPage(Uri uri);
    }
    
    private static class ZXTunesRemoteCatalog implements RemoteCatalog {
      @Override
      public boolean checkScheme(String scheme) {
        return scheme.equals("zxtunes");
      }
      
      @Override
      public String getPage(Uri uri) {
        final int author = Integer.parseInt(uri.getQueryParameter("author"));
        final int track = Integer.parseInt(uri.getQueryParameter("track"));
        return String.format(Locale.US, "http://zxtunes.com/author.php?id=%d&play=%d", author, track);
      }
    }
    
    private static class ZXArtRemoteCatalog implements RemoteCatalog {
      @Override
      public boolean checkScheme(String scheme) {
        return scheme.equals("zxart");
      }
      
      @Override
      public String getPage(Uri uri) {
        final int track = Integer.parseInt(uri.getQueryParameter("track"));
        return String.format(Locale.US, "http://zxart.ee/zxtune/action%%3aplay/tuneId%%3a%d", track);
      }
    }
  }
}
