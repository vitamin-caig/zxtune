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

import app.zxtune.sound.AsyncPlayback;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.IBinder;
import android.os.Messenger;
import android.util.Log;

public class PlaybackService extends Service {
    
  public final static String POSITION_UPDATE = "app.zxtune.playback.position_update";
  public final static String CURRENT_FRAME = "frame";
  public final static String TOTAL_FRAMES = "total";
  
  private final static String TAG = "app.zxtune.Service";

  private NotificationManager notificationManager;
  private Notification notification;
  private PendingIntent content;
  
  private final Playback.Control ctrl = new PlaybackControl();
  private final Messenger messenger = MessengerRPC.Server.createMessenger(ctrl);

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");
    this.notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
    this.notification = new Notification();
    notification.flags |= Notification.FLAG_NO_CLEAR;
    final Intent intent = new Intent(this, PlayFileActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    content = PendingIntent.getActivity(this, 0, intent, 0);
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    ctrl.stop();
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    final String action = intent != null ? intent.getAction() : null;
    if (action != null && action.equals(Intent.ACTION_VIEW)) {
      final String fileName = Uri.parse(intent.toUri(0)).getPath();
      if (fileName.length() != 0) {
        ctrl.open(fileName);
      }
    }
    return START_NOT_STICKY;
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return messenger.getBinder();
  }

  private void showPlayingNotification() {
    notification.icon = R.drawable.ic_stat_notify_play;
    showNotification();
  }

  private void showPausedNotification() {
    notification.icon = R.drawable.ic_stat_notify_pause;
    showNotification();
  }

  private void showNotification() {
    notification.when = System.currentTimeMillis();
    notificationManager.notify(R.string.app_name, notification);
  }

  private class PlaybackControl implements Playback.Control {
    
    private AsyncPlayback playback;
    
    public void open(String moduleId) {
      Log.d(TAG, String.format("Play %s", moduleId));
      final byte[] content = LoadFile(moduleId);
      final ZXTune.Data data = ZXTune.createData(content);
      final ZXTune.Module module = data.createModule();
      data.release();
      play(module);
    }

    public void play() {
      playback.play();
    }

    public void pause() {
      playback.pause();
    }

    public void stop() {
      Log.d(TAG, "Stop");
      if (playback != null) {
        playback.stop();
        playback = null;
      }
    }
    
    private void play(ZXTune.Module module) {
      stop();
      final String title = describeModule(module);
      notification.setLatestEventInfo(PlaybackService.this, getText(R.string.app_name), title, content);
      final AsyncPlayback.Source src = new PlaybackSource(module);
      final AsyncPlayback.Callback cb = new PlaybackCallback();
      playback = new AsyncPlayback(src);
      playback.setCallback(cb);
      play();
    }

    private String describeModule(ZXTune.Module module) {
      final String author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "Unknown");
      final String title = module.getProperty(ZXTune.Module.Attributes.TITLE, "Unknown");
      return author + " - " + title;
    }

    private byte[] LoadFile(String path) {
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
  }

  private class PlaybackSource implements AsyncPlayback.Source {

    private ZXTune.Module module;
    private ZXTune.Player player;
    private Intent progressUpdate;

    public PlaybackSource(ZXTune.Module module) {
      this.module = module;
      this.player = module.createPlayer();
      this.progressUpdate = new Intent(POSITION_UPDATE);
      progressUpdate.putExtra(TOTAL_FRAMES, module.getDuration());
    }

    @Override
    public void setFreqRate(int freqRate) {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, freqRate);
      player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, 1);
    }

    @Override
    public boolean getNextSoundChunk(byte[] buf) {
      progressUpdate.putExtra(CURRENT_FRAME, player.getPosition());
      sendBroadcast(progressUpdate);
      return player.render(buf);
    }

    @Override
    public void release() {
      player.release();
      module.release();
    }
  }

  private class PlaybackCallback implements AsyncPlayback.Callback {

    private final static String TAG = PlaybackService.TAG + ".AsyncPlaybackCallback";
    @Override
    public void onStart() {
      Log.d(TAG, "onStart");
      showPlayingNotification();
      startForeground(R.string.app_name, notification);
    }

    @Override
    public void onStop() {
      Log.d(TAG, "onStop");
      stopForeground(true);
    }

    @Override
    public void onPause() {
      Log.d(TAG, "onPause");
      showPausedNotification();
    }

    @Override
    public void onResume() {
      Log.d(TAG, "onResume");
      showPlayingNotification();
    }

    @Override
    public void onFinish() {
      Log.d(TAG, "onFinish");
    }
  }
}
