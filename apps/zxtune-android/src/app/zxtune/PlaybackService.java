/*
 * @file
 * @brief Background service class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.app.Service;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.rpc.BroadcastPlaybackCallback;
import app.zxtune.rpc.PlaybackControlServer;
import app.zxtune.ZXTune.Player;
import app.zxtune.playlist.Database;
import app.zxtune.sound.AsyncPlayback;
import app.zxtune.ui.StatusNotification;

public class PlaybackService extends Service {

  private final static String TAG = "app.zxtune.Service";

  private Playback.Control ctrl;
  private IBinder binder;

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");

    final Playback.Callback notification =
        new StatusNotification(this,
            new Intent(this, CurrentlyPlayingActivity.class)
                .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP));
    final Playback.Callback broadcast = new BroadcastPlaybackCallback(this);
    final Playback.Callback callback = new DoublePlaybackCallback(notification, broadcast);
    ctrl = new PlaybackControl(callback);
    binder = new PlaybackControlServer(ctrl);
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    ctrl.stop();
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "StartCommand called");
    final String action = intent != null ? intent.getAction() : null;
    final Uri uri = intent.getData();
    if (action != null && uri != Uri.EMPTY) {
      startAction(action, uri);
    }
    return START_STICKY;
  }

  private final void startAction(String action, Uri uri) {
    if (action.equals(Intent.ACTION_VIEW)) {
      Log.d(TAG, "Playing module " + uri);
      ctrl.play(uri);
    } else if (action.equals(Intent.ACTION_INSERT)) {
      /*
      Log.d(TAG, "Adding to playlist all modules from " + uri);
      */
    }
  }

  /*
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
  */

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }
  
  interface PlayableItem extends Playback.Item, Closeable {
    
    public ZXTune.Player createPlayer();
  }

  private PlayableItem openItem(Uri uri) throws IOException {
    final Uri dataUri = getDataUri(uri);
    final ZXTune.Module module = openModule(dataUri);
    return new ActiveItem(uri, dataUri, module);
  }

  private Uri getDataUri(Uri uri) {
    if (uri.getScheme().equals(ContentResolver.SCHEME_CONTENT)) {
      Log.d(TAG, " playlist reference scheme");
      return getPlaylistItemDataUri(uri);
    } else {
      Log.d(TAG, " direct data scheme");
      return uri;
    }
  }

  private Uri getPlaylistItemDataUri(Uri uri) {
    Cursor cursor = null;
    try {
      final String[] projection = {Database.Tables.Playlist.Fields.uri.name()};
      cursor = getContentResolver().query(uri, projection, null, null, null);
      if (cursor != null && cursor.moveToFirst()) {
        final String dataUri = cursor.getString(0);
        return Uri.parse(dataUri);
      }
      throw new RuntimeException();
    } finally {
      if (cursor != null) {
        cursor.close();
      }
    }
  }
  
  private static ZXTune.Module openModule(Uri path) throws IOException {
    final byte[] content = loadFile(path.getPath());
    final ZXTune.Data data = ZXTune.createData(content);
    final ZXTune.Module module = data.createModule();
    data.close();
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

  private static class DoublePlaybackCallback implements Playback.Callback {

    private final Playback.Callback first;
    private final Playback.Callback second;
    
    public DoublePlaybackCallback(Playback.Callback first, Playback.Callback second) {
      this.first = first;
      this.second = second;
    }

    @Override
    public void itemChanged(Playback.Item item) {
      first.itemChanged(item);
      second.itemChanged(item);
    }

    @Override
    public void statusChanged(Playback.Status status) {
      first.statusChanged(status);
      second.statusChanged(status);
    }
  }
  
  private static class ActiveItem implements PlayableItem {
    
    private ZXTune.Module module;
    private final Uri id;
    private final Uri dataId;
    private final String title;
    private final String author;
    private final TimeStamp duration;
    
    public ActiveItem(Uri id, Uri dataId, ZXTune.Module module) {
      this.module = module;
      this.id = id;
      this.dataId = dataId;
      this.title = module.getProperty(ZXTune.Module.Attributes.TITLE, "");
      this.author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
      //TODO
      this.duration = new TimeStamp(20 * module.getDuration(), TimeUnit.MILLISECONDS);
    }
    
    @Override
    public Uri getId() {
      return id;
    }

    @Override
    public Uri getDataId() {
      return dataId;
    }

    @Override
    public String getTitle() {
      return title;
    }

    @Override
    public String getAuthor() {
      return author;
    }

    @Override
    public TimeStamp getDuration() {
      return duration;
    }

    @Override
    public void close() throws IOException {
      if (module != null) {
        module.close();
        module = null;
      }
    }

    @Override
    public Player createPlayer() {
      return module.createPlayer();
    }
  }

  private class PlaybackControl implements Playback.Control, Playback.Callback {
    
    private final Playback.Callback callback;
    private AsyncPlayback playback;
    private Playback.Item item;
    private Playback.Status status;
    
    PlaybackControl(Playback.Callback callback) {
      this.callback = callback;
    }

    @Override
    public Playback.Item getItem() {
      return playback != null ? item : null;
    }

    @Override
    public Playback.Status getStatus() {
      return status;
    }

    @Override
    public void play(Uri uri) {
      Log.d(TAG, "play(" + uri + ")");
      try {
        stop();
        final PlayableItem item = openItem(uri);
        final AsyncPlayback.Source source = new PlaybackSource(item, this);
        final AsyncPlayback playback = new AsyncPlayback(source);
        this.playback = playback;
        play();
      } catch (IOException e) {
      }
    }

    @Override
    public void play() {
      Log.d(TAG, "play()");
      if (playback != null) {
        playback.resume();
      } else if (item != null) {
        play(item.getId());
      }
    }

    @Override
    public void pause() {
      Log.d(TAG, "pause()");
      if (playback != null) {
        playback.pause();
      }
    }

    @Override
    public void stop() {
      Log.d(TAG, "stop()");
      if (playback != null) {
        playback.stop();
        playback = null;
      }
    }

    @Override
    public void itemChanged(Playback.Item item) {
      this.item = item;
      callback.itemChanged(item);
    }

    @Override
    public void statusChanged(Playback.Status status) {
      this.status = status;
      callback.statusChanged(status);
    }
  }
  
  /*
  private static class CompositePlaybackSource implements AsyncPlayback.Source {
    
    private Enumeration<AsyncPlayback.Source> sources;
    private int freqRate;
    private AsyncPlayback.Source current;
    
    // @invariant sources should not be empty 
    public CompositePlaybackSource(Enumeration<AsyncPlayback.Source> sources) {
      this.sources = sources;
      this.current = sources.nextElement();
    }
    
    @Override
    public void startup(int freqRate) {
      this.freqRate = freqRate;
      current.startup(freqRate);
    }

    @Override
    public void suspend() {
      current.suspend();
    }

    @Override
    public void resume() {
      current.resume();
    }

    @Override
    public void shutdown() {
      current.shutdown();
    }
    
    @Override
    public boolean getNextSoundChunk(byte[] buf) {
      if (current.getNextSoundChunk(buf)) {
        return true;
      }
      return getNextSource();
    }

    @Override
    public void close() throws IOException {
      sources = null;
      closeCurrent();
      current = null;
    }
    
    private boolean getNextSource() {
      try {
        if (sources.hasMoreElements()) {
          closeCurrent();
          moveToNext();
          return true;
        }
      } catch (IOException e) {
      }
      return false;
    }
    
    private void closeCurrent() throws IOException {
      current.shutdown();
      current.close();
      current = null;
    }
    
    private void moveToNext() {
      current = sources.nextElement();
      current.startup(freqRate);
    }
  }
  */

  private static class PlaybackSource implements AsyncPlayback.Source, Playback.Status {

    private final PlayableItem item;
    private final Playback.Callback callback;
    private ZXTune.Player player;
    private boolean paused;

    public PlaybackSource(PlayableItem item, Playback.Callback callback) {
      this.item = item;
      this.callback = callback;
    }

    @Override
    public void startup(int freqRate) {
      player = item.createPlayer();
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, freqRate);
      player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, 1);
      callback.itemChanged(item);
      callback.statusChanged(this);
    }

    @Override
    public void suspend() {
      paused = true;
      callback.statusChanged(this);
    }

    @Override
    public void resume() {
      paused = false;
      callback.statusChanged(this);
    }

    @Override
    public void shutdown() {
      callback.statusChanged(null);
      try {
        player.close();
      } catch (IOException e) {
      } finally {
        player = null;
      }
    }
    
    @Override
    public boolean getNextSoundChunk(byte[] buf) {
      return player.render(buf);
    }

    @Override
    public TimeStamp getPosition() {
      final int frame = player.getPosition();
      //TODO
      return new TimeStamp(20 * frame, TimeUnit.MILLISECONDS);
    }

    @Override
    public boolean isPaused() {
      return paused;
    }
  }
}
