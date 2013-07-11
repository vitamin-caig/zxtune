/*
 * @file
 * @brief Background service class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.playback.Control;
import app.zxtune.playback.FileIterator;
import app.zxtune.playback.Item;
import app.zxtune.playback.Iterator;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.StubIterator;
import app.zxtune.playback.StubPlayableItem;
import app.zxtune.playlist.Query;
import app.zxtune.rpc.BroadcastPlaybackCallback;
import app.zxtune.rpc.PlaybackControlServer;
import app.zxtune.sound.AsyncPlayer;
import app.zxtune.sound.Player;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.StubPlayer;
import app.zxtune.sound.StubVisualizer;
import app.zxtune.sound.Visualizer;
import app.zxtune.ui.StatusNotification;

public class PlaybackService extends Service {

  private final static String TAG = PlaybackService.class.getName();

  private final Handler handler;
  private PlaybackControl ctrl;
  private IBinder binder;
  private PhoneCallHandler callHandler;
  
  public PlaybackService() {
    this.handler = new Handler();
  }

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
    callHandler = new PhoneCallHandler(this, ctrl);
    callHandler.register();
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    callHandler.unregister();
    callHandler = null;
    ctrl.release();
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
      addModuleToPlaylist(uri);
    }
  }

  private void addModuleToPlaylist(Uri uri) {
    try {
      final ZXTune.Module module = FileIterator.loadModule(uri);
      final app.zxtune.playlist.Item item = new app.zxtune.playlist.Item(uri, module);
      module.release();
      getContentResolver().insert(Query.unparse(null), item.toContentValues());
    } catch (IOException e) {
      Log.d(TAG, e.toString());
    }
  }


  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }

  private class PlaybackControl implements Control, Releaseable {

    private Callback callback;
    private Iterator iterator;
    private PlayableItem item;
    private Player player;
    private Visualizer visualizer;

    PlaybackControl(Callback callback) {
      this.callback = callback;
      this.iterator = StubIterator.instance();
      this.item = StubPlayableItem.instance();
      this.player = StubPlayer.instance();
      this.visualizer = StubVisualizer.instance();
      callback.onControlChanged(this);
    }

    @Override
    public Item getItem() {
      return item;
    }

    @Override
    public TimeStamp getPlaybackPosition() {
      return player.getPosition();
    }

    @Override
    public int[] getSpectrumAnalysis() {
      final int MAX_VOICES = 16;
      final int[] bands = new int[MAX_VOICES];
      final int[] levels = new int[MAX_VOICES];
      final int chans = visualizer.getSpectrum(bands, levels);
      final int[] result = new int[chans];
      for (int i = 0; i != chans; ++i) {
        result[i] = 256 * levels[i] + bands[i];
      }
      return result;
    }

    @Override
    public boolean isPlaying() {
      return player.isStarted();
    }

    @Override
    public void play(Uri uri) {
      Log.d(TAG, "play(" + uri + ")");
      try {
        final Iterator newIterator = Iterator.create(getApplicationContext(), uri);
        play(newIterator);
        iterator = newIterator;
      } catch (IOException e) {}
    }
    
    private void play(Iterator newIterator) throws IOException {
      final PlayableItem newItem = newIterator.getItem();
      final ZXTune.Player lowPlayer = newItem.createPlayer();
      final SamplesSource source = new PlaybackSamplesSource(lowPlayer);
      final PlayerEventsListener events = new PlaybackEvents(callback);
      final Visualizer newVisualizer = new PlaybackVisualizer(lowPlayer);

      release();
      player = AsyncPlayer.create(source, events);
      item = newItem;
      visualizer = newVisualizer;
      callback.onItemChanged(item);
      play();
    }

    @Override
    public void play() {
      player.startPlayback();
    }
    
    @Override
    public void stop() {
      player.stopPlayback();
    }
    
    @Override
    public void next() {
      try {
        if (iterator.next()) {
          play(iterator);
        }
      } catch (IOException e) {
        Log.d(TAG, e.toString());
      }
    }
    
    @Override
    public void prev() {
      try {
        if (iterator.prev()) {
          play(iterator);
        }
      } catch (IOException e) {
        Log.d(TAG, e.toString());
      }
    }
    
    @Override
    public void setPlaybackPosition(TimeStamp pos) {
      player.setPosition(pos);
    }
    
    @Override
    public void release() {
      stop();
      visualizer = StubVisualizer.instance();
      try {
        player.release();
      } finally {
        player = StubPlayer.instance();
      }
    }
  }
  
  private final class PlaybackEvents implements PlayerEventsListener {

    private final Callback callback;
    
    public PlaybackEvents(Callback callback) {
      this.callback = callback;
    }
    
    @Override
    public void onStart() {
      callback.onStatusChanged(true);
    }

    @Override
    public void onFinish() {
      handler.post(new Runnable() {
        @Override
        public void run() {
          ctrl.next();
        }
      });
    }

    @Override
    public void onStop() {
      callback.onStatusChanged(false);
    }

    @Override
    public void onError(Error e) {
      Log.d(TAG, "Error occurred: " + e);
    }
  }
  
  private static final class PlaybackSamplesSource implements SamplesSource {

    private ZXTune.Player player;
    private TimeStamp frameDuration;
    private volatile TimeStamp seekRequest;
    
    public PlaybackSamplesSource(ZXTune.Player player) {
      this.player = player;
      final long frameDurationUs = player.getProperty(ZXTune.Properties.Sound.FRAMEDURATION, ZXTune.Properties.Sound.FRAMEDURATION_DEFAULT); 
      this.frameDuration = TimeStamp.createFrom(frameDurationUs, TimeUnit.MICROSECONDS);
      player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, ZXTune.Properties.Core.Aym.INTERPOLATION_LQ);
      player.setPosition(0);
    }
    
    @Override
    public void initialize(int sampleRate) {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, sampleRate);
    }

    @Override
    public boolean getSamples(short[] buf) {
      if (seekRequest != null) {
        final int frame = (int) seekRequest.divides(frameDuration); 
        player.setPosition(frame);
        seekRequest = null;
      }
      return player.render(buf);
    }

    @Override
    public TimeStamp getPosition() {
      TimeStamp res = seekRequest;
      if (res == null) {
        final int frame = player.getPosition();
        res = frameDuration.multiplies(frame); 
      }
      return res;
    }

    @Override
    public void setPosition(TimeStamp pos) {
      seekRequest = pos;
    }
    
    @Override
    public void release() {
      player.release();
      player = null;
    }
  }
  
  private static final class PlaybackVisualizer implements Visualizer {
    
    private final ZXTune.Player player;
    
    public PlaybackVisualizer(ZXTune.Player player) {
      this.player = player;
    }

    @Override
    public int getSpectrum(int[] bands, int[] levels) {
      return player.analyze(bands, levels);
    }
  }
}
