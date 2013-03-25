/*
 * @file
 * 
 * @brief Background service class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.sound.AsyncPlayback;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.Query;

public class PlaybackService extends Service {

  private final static String TAG = "app.zxtune.Service";

  private final PlaybackControl ctrl = new PlaybackControl();
  private Playback.Callback callback;
  private final IBinder binder = MessengerRPC.ControlServer.createBinder(ctrl);

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");
    callback = new NotificationCallback();
    ctrl.registerCallback(callback);
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    ctrl.unregisterCallback(callback);
    ctrl.stop();
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    final String action = intent != null ? intent.getAction() : null;
    final Uri uri = intent.getData();
    if (action != null && uri != Uri.EMPTY) {
      startAction(action, uri);
    }
    return START_NOT_STICKY;
  }
  
  private final void startAction(String action, Uri uri) {
    if (action.equals(Intent.ACTION_VIEW)) {
      Log.d(TAG, "Playing module " + uri);
      playModule(uri);
    } else if (action.equals(Intent.ACTION_INSERT)) {
      Log.d(TAG, "Adding to playlist all modules from " + uri);
      final ZXTune.Module module = openModule(uri);
      addModuleToPlaylist(uri, module);
      module.release();
    }
  }
  
  private final void playModule(Uri uri) {
    final Uri dataUri = getDataUri(uri);
    final ZXTune.Module module = openModule(dataUri);
    ctrl.play(uri, module);
  }
   
  private final Uri getDataUri(Uri uri) {
    if (uri.getScheme().equals(ContentResolver.SCHEME_CONTENT)) {
      Log.d(TAG, " playlist reference scheme");
      return getPlaylistItemDataUri(uri);
    } else {
      Log.d(TAG, " direct data scheme");
      return uri;
    }
  }
  
  private final Uri getPlaylistItemDataUri(Uri uri) {
    Cursor cursor = null;
    try {
      final String[] projection = {Database.Tables.Playlist.Fields.uri.name()};
      cursor = getContentResolver().query(uri, projection, null, null, null);
      if (cursor != null && cursor.moveToFirst()) {
        final String dataUri = cursor.getString(0);
        return Uri.parse(dataUri);
      }
    } finally {
      if (cursor != null) {
        cursor.close();
      }
    }
    return null;
  }
  
  private void addModuleToPlaylist(Uri uri, ZXTune.Module module) {
    final String type = module.getProperty(ZXTune.Module.Attributes.TYPE, "");
    final String author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
    final String title = module.getProperty(ZXTune.Module.Attributes.TITLE, "");
    final int duration = module.getDuration() * 20;//TODO
    final ContentValues values = new ContentValues();
    values.put(Database.Tables.Playlist.Fields.uri.name(), uri.toString());
    values.put(Database.Tables.Playlist.Fields.type.name(), type);
    values.put(Database.Tables.Playlist.Fields.author.name(), author);
    values.put(Database.Tables.Playlist.Fields.title.name(), title);
    values.put(Database.Tables.Playlist.Fields.duration.name(), duration);
    getContentResolver().insert(Query.unparse(null), values);
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }

  private static ZXTune.Module openModule(Uri path) {
    final byte[] content = loadFile(path.getPath());
    final ZXTune.Data data = ZXTune.createData(content);
    final ZXTune.Module module = data.createModule();
    data.release();
    return module;
  }
  
  private static byte[] loadFile(String path) {
    try {
      final File file = new File(path);
      final FileInputStream stream = new FileInputStream(file);
      final int size = (int) file.length();
      byte[] result = new byte[size];
      stream.read(result, 0, size);
      return result;
    } catch (IOException e) {
      Log.d(TAG, e.toString());
      return null;
    }
  }
  
  private class NotificationCallback implements Playback.Callback {
    
    private NotificationManager notificationManager;
    private PendingIntent content;
    
    public NotificationCallback() {
      this.notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
      final Intent intent = new Intent(PlaybackService.this, CurrentlyPlayingActivity.class);
      intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
      content = PendingIntent.getActivity(PlaybackService.this, 0, intent, 0);
    }

    public void started(Uri playlistUri, String description, int duration) {
      startForeground(R.string.app_name, showNotification(R.drawable.ic_stat_notify_play, description));
    }

    public void paused(String description) {
      startForeground(R.string.app_name, showNotification(R.drawable.ic_stat_notify_pause, description));
    }

    public void stopped() {
      stopForeground(true);
    }

    public void positionChanged(int curFrame, String curTime) {

    }
    
    private Notification showNotification(int icon, String description) {
      final Notification notification = new Notification(icon, description, System.currentTimeMillis());
      notification.setLatestEventInfo(PlaybackService.this, getString(R.string.app_name),
          description, content);
      notification.flags |= Notification.FLAG_NO_CLEAR;
      notificationManager.notify(R.string.app_name, notification);
      return notification;
    }
  }

  private static class PlaybackControl implements Playback.Control {

    private final CompositeCallback callback = new CompositeCallback();
    private AsyncPlayback playback;
    private Uri nowPlaying;
    
    public void play(Uri moduleUri, ZXTune.Module module) {
      stop();
      final String description = describeModule(module);
      final AsyncPlayback.Source src = new PlaybackSource(module.createPlayer(), callback);
      final AsyncPlayback.Callback cb =
          new PlaybackCallback(callback, moduleUri, description, module.getDuration());
      module.release();
      playback = new AsyncPlayback(src);
      playback.setCallback(cb);
      nowPlaying = moduleUri;
      play();
    }
    
    @Override
    public Uri nowPlaying() {
      return nowPlaying;
    }

    @Override
    public void play() {
      playback.play();
    }

    @Override
    public void pause() {
      playback.pause();
    }

    @Override
    public void stop() {
      Log.d(TAG, "Stop");
      if (playback != null) {
        playback.stop();
        playback = null;
      }
    }

    @Override
    public void registerCallback(Playback.Callback cb) {
      callback.add(cb);
    }

    @Override
    public void unregisterCallback(Playback.Callback cb) {
      callback.delete(cb);
    }

    private String describeModule(ZXTune.Module module) {
      final String author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "Unknown");
      final String title = module.getProperty(ZXTune.Module.Attributes.TITLE, "Unknown");
      return author + " - " + title;
    }

    private static class CompositeCallback implements Playback.Callback {
      private ArrayList<Playback.Callback> delegates = new ArrayList<Playback.Callback>();

      public void started(Uri playlistUri, String description, int duration) {
        for (Playback.Callback cb : delegates) {
          cb.started(playlistUri, description, duration);
        }
      }

      public void paused(String description) {
        for (Playback.Callback cb : delegates) {
          cb.paused(description);
        }
      }

      public void stopped() {
        for (Playback.Callback cb : delegates) {
          cb.stopped();
        }
      }

      public void positionChanged(int curFrame, String curTime) {
        for (Playback.Callback cb : delegates) {
          cb.positionChanged(curFrame, curTime);
        }
      }

      public void add(Playback.Callback cb) {
        delegates.add(cb);
      }

      public void delete(Playback.Callback cb) {
        delegates.remove(cb);
      }
    }
  }

  private static class PlaybackSource implements AsyncPlayback.Source {

    private ZXTune.Player player;
    private Playback.Callback callback;

    public PlaybackSource(ZXTune.Player player, Playback.Callback callback) {
      this.player = player;
      this.callback = callback;
    }

    @Override
    public void setFreqRate(int freqRate) {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, freqRate);
      player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, 1);
    }

    @Override
    public boolean getNextSoundChunk(byte[] buf) {
      callback.positionChanged(player.getPosition(), "");
      return player.render(buf);
    }

    @Override
    public void release() {
      player.release();
    }
  }

  private static class PlaybackCallback implements AsyncPlayback.Callback {

    private final static String TAG = PlaybackService.TAG + ".AsyncPlaybackCallback";

    private final Playback.Callback callback;
    private final Uri uri;
    private final String description;
    private final int duration;

    public PlaybackCallback(Playback.Callback callback, Uri uri, String description, int duration) {
      this.callback = callback;
      this.uri = uri;
      this.description = description;
      this.duration = duration;
    }

    public void onStart() {
      Log.d(TAG, "onStart");
      callback.started(uri, description, duration);
    }

    public void onStop() {
      Log.d(TAG, "onStop");
      callback.stopped();
    }

    public void onPause() {
      Log.d(TAG, "onPause");
      callback.paused(description);
    }

    public void onResume() {
      Log.d(TAG, "onResume");
      callback.started(uri, description, duration);
    }

    public void onFinish() {
      Log.d(TAG, "onFinish");
    }
  }
}
