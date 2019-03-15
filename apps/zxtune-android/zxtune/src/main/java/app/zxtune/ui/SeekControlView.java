/**
 * @file
 * @brief Seek control view logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Handler;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.models.MediaSessionModel;

import java.util.concurrent.TimeUnit;

class SeekControlView {

  private static final String TAG = SeekControlView.class.getName();

  private final TextView currentTime;
  private final TextView totalTime;
  private final SeekBar currentPosition;
  private final Handler timer;
  private final UpdateViewTask updateTask;

  SeekControlView(@NonNull final FragmentActivity activity, @NonNull View layout) {
    this.currentTime = layout.findViewById(R.id.position_time);
    this.totalTime = layout.findViewById(R.id.position_duration);
    this.currentPosition = layout.findViewById(R.id.position_seek);
    this.timer = new Handler();
    this.updateTask = new UpdateViewTask();
    final MediaSessionModel model = ViewModelProviders.of(activity).get(MediaSessionModel.class);
    this.currentPosition.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override
      public void onProgressChanged(SeekBar seekBar, int seconds, boolean fromUser) {
        final MediaControllerCompat ctrl = MediaControllerCompat.getMediaController(activity);
        if (fromUser && ctrl != null) {
          ctrl.getTransportControls().seekTo(seconds * 1000);
        }
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {}

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {}
    });
    model.getMetadata().observe(activity, new Observer<MediaMetadataCompat>() {
      @Override
      public void onChanged(@Nullable MediaMetadataCompat mediaMetadataCompat) {
        if (mediaMetadataCompat != null) {
          setDuration(mediaMetadataCompat.getLong(MediaMetadataCompat.METADATA_KEY_DURATION));
        }
      }
    });
    model.getState().observe(activity, new Observer<PlaybackStateCompat>() {
      @Override
      public void onChanged(@Nullable PlaybackStateCompat state) {
        if (state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING) {
          final long pos = state.getPosition();
          final float speed = state.getPlaybackSpeed();
          final long ts = state.getLastPositionUpdateTime();
          updateTask.start(pos, speed, ts);
        } else {
          updateTask.stop();
        }
      }
    });
  }

  private void setDuration(long ms) {
    final TimeStamp ts = TimeStamp.createFrom(ms, TimeUnit.MILLISECONDS);
    totalTime.setText(ts.toString());
    currentPosition.setMax((int) ts.convertTo(TimeUnit.SECONDS));
  }

  final void stop() {
    updateTask.stop();
  }

  private class UpdateViewTask implements Runnable {

    private long posMs;
    private float speed;
    private long ts;

    @Override
    public void run() {
      try {
        final TimeStamp pos = getNewPos();
        currentPosition.setProgress((int) pos.convertTo(TimeUnit.SECONDS));
        currentTime.setText(pos.toString());

        timer.postDelayed(this, 1000);
      } catch (Exception e) {
        Log.w(TAG, e, "updateViewTask.run()");
      }
    }

    private TimeStamp getNewPos() {
      final long now = SystemClock.elapsedRealtime();
      final long newPosMs = posMs + (long) ((now - ts) * speed);
      return TimeStamp.createFrom(newPosMs, TimeUnit.MILLISECONDS);
    }

    final void start(long pos, float speed, long ts) {
      this.posMs = pos;
      this.speed = speed;
      this.ts = ts;
      run();
    }

    final void stop() {
      timer.removeCallbacks(this);
    }
  }
}
