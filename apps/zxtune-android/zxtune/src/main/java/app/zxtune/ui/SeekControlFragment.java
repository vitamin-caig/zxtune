/**
 * @file
 * @brief Seek control view logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.device.media.MediaModel;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.ui.utils.UiUtils;

public class SeekControlFragment extends Fragment {

  private static final String TAG = SeekControlFragment.class.getName();

  private final Handler timer;
  private final UpdateViewTask updateTask;

  @Nullable
  private TextView currentTime;
  @Nullable
  private SeekBar currentPosition;
  @Nullable
  private TextView totalTime;
  @Nullable
  private ImageView trackMode;
  @Nullable
  private MediaControllerCompat.TransportControls ctrl;
  private int trackModeValue = 0;

  public SeekControlFragment() {
    this.timer = new Handler();
    this.updateTask = new UpdateViewTask();
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    final MediaModel model = MediaModel.of(requireActivity());
    model.getController().observe(this, controller -> {
      if (controller != null) {
        ctrl = controller.getTransportControls();
        trackModeValue = controller.getRepeatMode();
        updateTrackModeStatus();
      } else {
        ctrl = null;
      }
      UiUtils.setViewEnabled(getView(), controller != null);
    });
    model.getMetadata().observe(this, mediaMetadataCompat -> {
      if (mediaMetadataCompat != null) {
        setDuration(mediaMetadataCompat.getLong(MediaMetadataCompat.METADATA_KEY_DURATION));
      } else {
        setUnknownDuration();
      }
    });
    model.getPlaybackState().observe(this, state -> {
      if (state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING) {
        final long pos = state.getPosition();
        final float speed = state.getPlaybackSpeed();
        final long ts = state.getLastPositionUpdateTime();
        updateTask.start(pos, speed, ts);
      } else {
        updateTask.stop();
      }
    });
  }

  @Override
  @Nullable
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {
    return inflater.inflate(R.layout.position, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);

    currentTime = view.findViewById(R.id.position_time);
    currentPosition = view.findViewById(R.id.position_seek);
    currentPosition.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (ctrl != null && fromUser) {
          ctrl.seekTo(progress * 1000);
        }
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {

      }

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {

      }
    });
    totalTime = view.findViewById(R.id.position_duration);
    trackMode = view.findViewById(R.id.controls_track_mode);
    trackMode.setOnClickListener(v -> toggleTrackMode());
  }

  @Override
  public void onStop() {
    super.onStop();
    updateTask.stop();
  }

  private void toggleTrackMode() {
    trackModeValue = (trackModeValue + 1) % PlaybackControl.TrackMode.values().length;
    ctrl.setRepeatMode(trackModeValue);
    updateTrackModeStatus();
  }

  private void setDuration(long ms) {
    final TimeStamp ts = TimeStamp.fromMilliseconds(ms);
    totalTime.setText(ts.toString());
    currentPosition.setMax((int) ts.toSeconds());
  }

  private void setUnknownDuration() {
    totalTime.setText(R.string.stub_time);
    currentTime.setText(R.string.stub_time);
  }

  private void updateTrackModeStatus() {
    final boolean looped = trackModeValue == PlaybackControl.TrackMode.LOOPED.ordinal();
    trackMode.setImageResource(looped ? R.drawable.ic_track_looped : R.drawable.ic_track_regular);
  }

  private class UpdateViewTask implements Runnable {

    private long posMs;
    private float speed;
    private long ts;

    @Override
    public void run() {
      try {
        final TimeStamp pos = getNewPos();
        currentPosition.setProgress((int) pos.toSeconds());
        currentTime.setText(pos.toString());

        timer.postDelayed(this, 1000);
      } catch (Exception e) {
        Log.w(TAG, e, "updateViewTask.run()");
      }
    }

    private TimeStamp getNewPos() {
      final long now = SystemClock.elapsedRealtime();
      final long newPosMs = posMs + (long) ((now - ts) * speed);
      return TimeStamp.fromMilliseconds(newPosMs);
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
