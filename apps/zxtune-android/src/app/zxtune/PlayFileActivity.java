/*
* @file
* @brief Activity to play single file
* @version $Id:$
* @author (C) Vitamin/CAIG
*/

package app.zxtune;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.util.Log;
import java.nio.ByteBuffer;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class PlayFileActivity extends Activity {

  static final private String TAG = "zxtune";

  private PlayTask task;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.play_file);

    task = (PlayTask)getLastNonConfigurationInstance();
    if (task != null) {
      Log.d(TAG, "Have retained task");
    }
    if (task == null || task.getStatus() == AsyncTask.Status.FINISHED || task.isCancelled()) {
      task = new PlayTask("/mnt/sdcard/Speccy2.pt3");
    }
    else
    {
      Log.d(TAG, "Retain task");
      ((Button)findViewById(R.id.play_button)).setEnabled(false);
    }
    task.setHost(this);
  }

  public void onClick(View v) {
    switch (v.getId()) {
    case R.id.play_button:
      task.execute();
      break;
    case R.id.stop_button:
      task.cancel(false);
      break;
    }
  }

  @Override
  public Object onRetainNonConfigurationInstance() {
    Log.d(TAG, "Retaining current task");
    task.setHost(null);
    return task;
  }

  static class PlayTask extends AsyncTask<Void, Integer, Void> {

    private final static int SoundFreq = 44100;
    private final static int SoundChannels = AudioFormat.CHANNEL_CONFIGURATION_STEREO;
    private final static int SoundFormat = AudioFormat.ENCODING_PCM_16BIT;

    private PlayFileActivity host;
    private ZXTune.Module module;
    private ZXTune.Player player;
    private AudioTrack track;
    private ByteBuffer buf;

    PlayTask(String filename) {
      Log.d(TAG, "Load file " + filename);
      final ByteBuffer content = LoadFile(filename);
      Log.d(TAG, "Create data");
      final ZXTune.Data data = new ZXTune.Data(content);
      Log.d(TAG, "Create module");
      module = data.CreateModule();
      Log.d(TAG, "Create player");
      player = module.CreatePlayer();

      final int minBufferSize = AudioTrack.getMinBufferSize(SoundFreq, SoundChannels, SoundFormat);
      final int secondBufferSize = 4 * SoundFreq;
      final int bufferSize = Math.max(minBufferSize, secondBufferSize);
      Log.d(TAG, String.format("Buffers: min=%d, sec=%d", minBufferSize, secondBufferSize));
      track = new AudioTrack(AudioManager.STREAM_MUSIC, SoundFreq, SoundChannels, SoundFormat, bufferSize, AudioTrack.MODE_STREAM);
      buf = ByteBuffer.allocateDirect(bufferSize);
    }

    public void setHost(PlayFileActivity act) {
      host = act;
    }

    @Override
    protected void onPreExecute() {
      super.onPreExecute();
      ((Button)host.findViewById(R.id.play_button)).setEnabled(false);
    }

    @Override
    protected void onPostExecute(Void result) {
      super.onPostExecute(result);
      if (host != null) {
        ((Button)host.findViewById(R.id.play_button)).setEnabled(true);
      }
    }

    @Override
    protected void onCancelled() {
      super.onCancelled();
      if (host != null) {
        ((Button)host.findViewById(R.id.play_button)).setEnabled(true);
      }
    }

    @Override
    protected Void doInBackground(Void... params) {
      try {
        Log.d(TAG, "Started playback");
        final Integer[] progress = {0, module.GetFramesCount()};
        Log.d(TAG, String.format("Total %d frames", progress[1].intValue()));
        track.play();
        final byte[] rawBuf = new byte[buf.capacity()];
        while (!isCancelled() && player.Render(buf))
        {
          buf.get(rawBuf);
          buf.rewind();
          track.write(rawBuf, 0, rawBuf.length);
          progress[0] = player.GetPosition();
          publishProgress(progress);
        }
        Log.d(TAG, "Stopped");
        track.stop();
      }
      catch (Exception e) {
        Log.d(TAG, "Exception: " + e.toString());
      }
      return null;
    }

    @Override
    protected void onProgressUpdate(Integer... values) {
      super.onProgressUpdate(values);
      if (host != null) {
        ProgressBar bar = (ProgressBar)host.findViewById(R.id.play_position);
        bar.setProgress(values[0]);
        bar.setMax(values[1]);
      }
    }

    private static ByteBuffer LoadFile(String path) {
      try {
        final File file = new File(path);
        final FileInputStream stream = new FileInputStream(file);
        final int size = (int)file.length();
        byte[] buf = new byte[size];
        stream.read(buf, 0, size);
        final ByteBuffer result = ByteBuffer.allocateDirect(size);
        result.put(buf);
        return result;
      }
      catch (IOException e) {
        Log.d(TAG, e.toString());
        return null;
      }
    }
  }
}