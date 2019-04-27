package app.zxtune.ui.controllers;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.models.MediaSessionModel;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.stubs.VisualizerStub;
import app.zxtune.ui.views.SpectrumAnalyzerView;

import java.lang.ref.WeakReference;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

public class VisualizerController {

  private static final long PERIOD = 100;

  private final ScheduledExecutorService executorService;
  private final Handler handler;
  private final UpdateUiTask task;
  private Visualizer source;
  private WeakReference<SpectrumAnalyzerView> viewRef;
  private ScheduledFuture<?> future;

  public VisualizerController(FragmentActivity activity, SpectrumAnalyzerView view) {
    this.executorService = Executors.newSingleThreadScheduledExecutor();
    this.handler = new Handler();
    this.task = new UpdateUiTask();
    this.source = VisualizerStub.instance();
    this.viewRef = new WeakReference<>(view);

    final MediaSessionModel model = ViewModelProviders.of(activity).get(MediaSessionModel.class);
    model.getVisualizer().observe(activity, new Observer<Visualizer>() {
      @Override
      public void onChanged(@Nullable Visualizer visualizer) {
        source = visualizer != null ? visualizer : VisualizerStub.instance();
        if (visualizer == null) {
          stopUpdates();
        }
      }
    });
    model.getState().observe(activity, new Observer<PlaybackStateCompat>() {
      @Override
      public void onChanged(@Nullable PlaybackStateCompat state) {
        final boolean isCurrentlyPlaying = future != null;
        final boolean isPlaying = state != null && state.getState() == PlaybackStateCompat.STATE_PLAYING;
        if (isCurrentlyPlaying != isPlaying) {
          if (isPlaying) {
            startUpdates();
          } else {
            stopUpdates();
          }
        }
      }
    });
  }

  private void startUpdates() {
    stopUpdates();
    if (!executorService.isShutdown()) {
      future = executorService.scheduleAtFixedRate(
          new Runnable() {
            @Override
            public void run() {
              try {
                task.execute(source);
              } catch (Exception e) {
                stopUpdates();
              }
            }
          },
          PERIOD, PERIOD, TimeUnit.MILLISECONDS
      );
    }
  }

  private void stopUpdates() {
    if (future != null) {
      future.cancel(false);
      future = null;
      task.reset();
    }
  }

  public final void shutdown() {
    stopUpdates();
    executorService.shutdown();
  }

  private class UpdateUiTask implements Runnable {

    private final byte[] levels;
    private int channels;
    private AtomicBoolean scheduled;

    UpdateUiTask() {
      this.levels = new byte[SpectrumAnalyzerView.MAX_BANDS];
      this.scheduled = new AtomicBoolean();
    }

    final void execute(Visualizer src) throws Exception {
      this.channels = src.getSpectrum(levels);
      if (scheduled.compareAndSet(false, true)) {
        handler.post(this);
      }
    }

    final void reset() {
      channels = 0;
    }

    @Override
    public void run() {
      scheduled.set(false);
      final SpectrumAnalyzerView view = viewRef.get();
      if (view != null && view.update(channels, levels)) {
        channels = 0;
        if (scheduled.compareAndSet(false, true)) {
          handler.postDelayed(this, PERIOD);
        }
      }
    }
  }
}
