/*
 * @file
 * @brief Background service class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.app.Service;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.ZXTune.Player;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Status;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.Query;
import app.zxtune.rpc.BroadcastPlaybackCallback;
import app.zxtune.rpc.PlaybackControlServer;
import app.zxtune.sound.AsyncPlayback;
import app.zxtune.ui.StatusNotification;

public class PlaybackService extends Service {

  private final static String TAG = PlaybackService.class.getName();

  private PlaybackControl ctrl;
  private IBinder binder;
  private IncomingCallHandler callHandler; 

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");

    final Intent intent = new Intent(this, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    final CompositeCallback callback = new CompositeCallback();
    final StatusNotification notification = new StatusNotification(this, intent);
    final BroadcastPlaybackCallback broadcast = new BroadcastPlaybackCallback(this);
    callback.add(notification).add(broadcast);
    ctrl = new PlaybackControl(callback);
    binder = new PlaybackControlServer(ctrl);
    callHandler = new IncomingCallHandler(this, ctrl);
    callHandler.register();
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    callHandler.unregister();
    callHandler = null;
    ctrl.stop();
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "StartCommand called");
    final String action = intent != null ? intent.getAction() : null;
    final Uri uri = intent != null ? intent.getData() : Uri.EMPTY;
    if (action != null && uri != Uri.EMPTY) {
      startAction(action, uri);
    }
    return START_NOT_STICKY;
  }

  private final void startAction(String action, Uri uri) {
    if (action.equals(Intent.ACTION_VIEW)) {
      Log.d(TAG, "Playing module " + uri);
      ctrl.play(uri);
    } else if (action.equals(Intent.ACTION_INSERT)) {
      Log.d(TAG, "Adding to playlist all modules from " + uri);
      final Uri dataUri = getDataUri(uri);
      final ZXTune.Module module = openModule(dataUri);
      addModuleToPlaylist(uri, module);
    }
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

  interface PlayableItem extends Item, Releaseable {

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
      stream.close();
      return result;
    } catch (IOException e) {
      Log.d(TAG, e.toString());
      return null;
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
    public void release() {
      try {
        if (module != null) {
          module.release();
        }
      } finally {
        module = null;
      }
    }

    @Override
    public Player createPlayer() {
      return module.createPlayer();
    }
  }

  private class PlaybackControl implements Control {

    private final Callback callback;
    private PlaybackSource source;
    private AsyncPlayback playback;

    PlaybackControl(Callback callback) {
      this.callback = callback;
      callback.onControlChanged(this);
    }

    @Override
    public Item getItem() {
      return source != null ? source.getItem() : null;
    }

    @Override
    public TimeStamp getPlaybackPosition() {
      return source != null ? source.getPosition() : null;
    }

    @Override
    public int[] getSpectrumAnalysis() {
      return source != null ? source.getAnalysis() : null;
    }

    @Override
    public Status getStatus() {
      return source != null ? source.getStatus() : Status.STOPPED;
    }

    @Override
    public void play(Uri uri) {
      Log.d(TAG, "play(" + uri + ")");
      try {
        stop();
        final PlayableItem item = openItem(uri);
        source = new PlaybackSource(item, callback);
        final AsyncPlayback playback = new AsyncPlayback(source);
        this.playback = playback;
        play();
      } catch (IOException e) {}
    }

    @Override
    public void play() {
      Log.d(TAG, "play()");
      if (playback != null) {
        playback.resume();
      } else if (source != null) {
        play(source.getItem().getId());
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
  }

  private static class PlaybackSource implements AsyncPlayback.Source {

    private final PlayableItem item;
    private final Callback callback;
    private Status status;
    private ZXTune.Player player;

    public PlaybackSource(PlayableItem item, Callback callback) {
      this.item = item;
      this.callback = callback;
      this.status = Status.STOPPED;
      callback.onItemChanged(item);
    }

    @Override
    public void startup(int freqRate) {
      synchronized (status) {
        player = item.createPlayer();
        player.setProperty(ZXTune.Properties.Sound.FREQUENCY, freqRate);
        player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, 1);
        callback.onStatusChanged(status = Status.PLAYING);
      }
    }

    @Override
    public void suspend() {
      synchronized (status) {
        callback.onStatusChanged(status = Status.PAUSED);
      }
    }

    @Override
    public void resume() {
      synchronized (status) {
        callback.onStatusChanged(status = Status.PLAYING);
      }
    }

    @Override
    public void shutdown() {
      synchronized (status) {
        callback.onStatusChanged(status = Status.STOPPED);
        try {
          player.release();
        } finally {
          player = null;
        }
      }
    }

    @Override
    public boolean getNextSoundChunk(short[] buf) {
      return player.render(buf);
    }

    public Item getItem() {
      return item;
    }

    public Status getStatus() {
      return status;
    }

    public TimeStamp getPosition() {
      synchronized (status) {
        final int frame = player != null ? player.getPosition() : 0;
        //TODO
        return new TimeStamp(20 * frame, TimeUnit.MILLISECONDS);
      }
    }

    public int[] getAnalysis() {
      synchronized (status) {
        return player != null ? getAnalysis(player) : null;
      }
    }

    static private int[] getAnalysis(ZXTune.Player player) {
      final int[] bands = new int[8];
      final int[] levels = new int[8];
      final int size = player.analyze(bands, levels);
      final int[] result = new int[size];
      for (int i = 0; i != size; ++i) {
        result[i] = 256 * levels[i] + bands[i];
      }
      return result;
    }
  }
}
