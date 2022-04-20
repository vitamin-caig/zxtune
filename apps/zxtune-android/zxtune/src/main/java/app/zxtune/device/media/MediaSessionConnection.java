package app.zxtune.device.media;

import android.content.ComponentName;
import android.support.v4.media.MediaBrowserCompat;
import android.support.v4.media.session.MediaControllerCompat;

import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;

import app.zxtune.Log;
import app.zxtune.MainService;

public class MediaSessionConnection {

  private final static String TAG = MediaSessionConnection.class.getName();

  private final FragmentActivity activity;
  private final MediaBrowserCompat browser;
  private boolean connectionInProgress = false;

  public MediaSessionConnection(FragmentActivity activity) {
    this.activity = activity;
    this.browser = new MediaBrowserCompat(activity, new ComponentName(activity, MainService.class),
        new ConnectionCallback(), null);
  }

  public final void connect() {
    if (!connectionInProgress) {
      connectionInProgress = true;
      browser.connect();
    }
  }

  public final void disconnect() {
    browser.disconnect();
    setControl(null);
  }

  private void setControl(@Nullable MediaControllerCompat ctrl) {
    final MediaSessionModel model = MediaSessionModel.of(activity);
    model.setControl(ctrl);
    MediaControllerCompat.setMediaController(activity, ctrl);
    connectionInProgress = false;
  }

  private class ConnectionCallback extends MediaBrowserCompat.ConnectionCallback {
    @Override
    public void onConnected() {
      Log.d(TAG, "Connected");
      try {
        final MediaControllerCompat ctrl = new MediaControllerCompat(activity, browser.getSessionToken());
        setControl(ctrl);
      } catch (Exception e) {
        Log.w(TAG, e, "Failed to connect to MediaBrowser");
        onConnectionFailed();
      }
    }

    @Override
    public void onConnectionSuspended() {
      Log.d(TAG, "Connection suspended");
      onConnectionFailed();
    }

    @Override
    public void onConnectionFailed() {
      setControl(null);
    }
  }
}
