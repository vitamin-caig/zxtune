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

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

public class Service extends android.app.Service {
  
  public final static String POSITION_UPDATE = "app.zxtune.playback.position_update";
  public final static String CURRENT_FRAME = "frame";
  public final static String TOTAL_FRAMES = "total";
  public final static String FILE_NAME = "filename";

  private final static String TAG = "zxtune";

  private Playback playback;
  
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
    final ZXTune.Data data = new ZXTune.Data(content);
    final ZXTune.Module module = data.createModule();
    if (playback != null) {
      playback.stop();
    }
    showNotification(module);
    playback = new Playback(module);
    new Thread(playback).start();
  }
  
  private void showNotification(ZXTune.Module module) {
    final String description = describeModule(module);
    final Notification note = new Notification(android.R.drawable.ic_media_play, "Playing", System.currentTimeMillis());
    final Intent intent = new Intent(this, PlayFileActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    final PendingIntent pi = PendingIntent.getActivity(this, 0, intent, 0);
    note.setLatestEventInfo(this, "ZXTune", description, pi);
    note.flags|=Notification.FLAG_NO_CLEAR;
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
  
  private class Playback implements Runnable {

    private final static int STREAM_TYPE = AudioManager.STREAM_MUSIC;
    private final static int CHANNELS = AudioFormat.CHANNEL_OUT_STEREO;
    private final static int ENCODING = AudioFormat.ENCODING_PCM_16BIT;
    private final static int PLAYBACK_LATENCY_MS = 500;

    private ZXTune.Module module;
    private ZXTune.Player player;
    private byte[] buffer;
    private AudioTrack output;

    public Playback(ZXTune.Module mod) {
      module = mod;
      player = module.createPlayer();
      final int soundFreq = AudioTrack.getNativeOutputSampleRate(STREAM_TYPE);
      final int bufSize = getBufferSize(soundFreq);
      buffer = new byte[bufSize];
      output = new AudioTrack(STREAM_TYPE, soundFreq, CHANNELS, ENCODING, bufSize, AudioTrack.MODE_STREAM);
      player.setProperty(ZXTune.Properties.SOUND_FREQUENCY, soundFreq);
      Log.d(TAG, String.format("Initialized playback. Freq=%dHz Buf=%d samples", soundFreq, bufSize / 4));
    }

    @Override
    public void run() {
      Log.d(TAG, "Started playback");
      final Intent intent = new Intent(POSITION_UPDATE);
      intent.putExtra(TOTAL_FRAMES, module.getFramesCount());
      output.play();
      while (!isStopped()) {
        final boolean cont = player.render(buffer);
        output.write(buffer, 0, buffer.length);
        if (!cont) {
          output.stop();
          break;
        }
        intent.putExtra(CURRENT_FRAME, player.getPosition());
        sendBroadcast(intent);
      }
      Log.d(TAG, "Stopped playback");
    }
    
    public void stop() {
      if (isStopped()) {
        return;
      }
      output.stop();
    }
    
    private boolean isStopped() {
      return AudioTrack.PLAYSTATE_STOPPED == output.getPlayState();
    }

    private int getBufferSize(int sndFreq) {
      final int minBufSize =
          AudioTrack.getMinBufferSize(sndFreq, CHANNELS, ENCODING);
      final int reqBufSize = 4 * (sndFreq * PLAYBACK_LATENCY_MS / 1000);
      return Math.max(minBufSize, reqBufSize);
    }
  }
}
