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
import android.app.PendingIntent;
import android.content.Intent;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

public class Service extends android.app.Service {

  public final static String POSITION_UPDATE = "app.zxtune.playback.position_update";
  public final static String CURRENT_FRAME = "frame";
  public final static String TOTAL_FRAMES = "total";
  public final static String FILE_NAME = "filename";

  private final static String TAG = "zxtune";

  private AsyncPlayback playback;

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    final String fileName = intent.getStringExtra(FILE_NAME);
    if (!TextUtils.isEmpty(fileName)) {
      play(fileName);
    }
    return START_NOT_STICKY;
  }

  @Override
  public void onDestroy() {
    stop();
  }

  @Override
  public IBinder onBind(Intent intent) {
    return null;
  }

  private void play(String fileName) {
    Log.d(TAG, String.format("Play %s", fileName));
    final byte[] content = LoadFile(fileName);
    final ZXTune.Data data = ZXTune.createData(content);
    final ZXTune.Module module = data.createModule();
    data.release();
    if (playback != null) {
      stop();
    }
    showNotification(module);
    final AsyncPlayback.Source src = new PlaybackSource(module);
    playback = new AsyncPlayback(src);
  }

  private void showNotification(ZXTune.Module module) {
    final String description = describeModule(module);
    final Notification note =
        new Notification(android.R.drawable.ic_media_play, "Playing", System.currentTimeMillis());
    final Intent intent = new Intent(this, PlayFileActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    final PendingIntent pi = PendingIntent.getActivity(this, 0, intent, 0);
    note.setLatestEventInfo(this, "ZXTune", description, pi);
    note.flags |= Notification.FLAG_NO_CLEAR;
    startForeground(1337, note);
  }

  private static String describeModule(ZXTune.Module module) {
    final String author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "Unknown");
    final String title = module.getProperty(ZXTune.Module.Attributes.TITLE, "Unknown");
    return author + " - " + title;
  }

  private void stop() {
    Log.d(TAG, "Stop");
    if (playback != null) {
      playback.stop();
      playback = null;
    }
    stopForeground(true);
  }

  private static byte[] LoadFile(String path) {
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

  private static class PlaybackSource implements AsyncPlayback.Source {

    private ZXTune.Module module;
    private ZXTune.Player player;

    public PlaybackSource(ZXTune.Module module) {
      this.module = module;
      this.player = module.createPlayer();
    }

    @Override
    public void setFreqRate(int freqRate) {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, freqRate);
      player.setProperty(ZXTune.Properties.Core.Aym.INTERPOLATION, 1);
    }

    @Override
    public boolean getNextSoundChunk(byte[] buf) {
      return player.render(buf);
    }

    @Override
    public void release() {
      player.release();
      module.release();
    }
  };
}
