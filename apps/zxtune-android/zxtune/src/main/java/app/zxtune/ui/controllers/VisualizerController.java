package app.zxtune.ui.controllers;

import android.os.Handler;

import java.lang.ref.WeakReference;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import app.zxtune.playback.Visualizer;
import app.zxtune.playback.stubs.VisualizerStub;
import app.zxtune.ui.views.SpectrumAnalyzerView;

public class VisualizerController {

    private static final long PERIOD = 100;

    private final ScheduledExecutorService executorService;
    private final Handler handler;
    private final UpdateUiTask task;
    private Visualizer source;
    private WeakReference<SpectrumAnalyzerView> viewRef;
    private ScheduledFuture<?> future;

    public VisualizerController() {
        this.executorService = Executors.newSingleThreadScheduledExecutor();
        this.handler = new Handler();
        this.task = new UpdateUiTask();
        this.source = VisualizerStub.instance();
        this.viewRef = new WeakReference<>(null);
    }

    public final void setSource(Visualizer source) {
        this.source = source;
    }

    public final void setView(SpectrumAnalyzerView view) {
        this.viewRef = new WeakReference<>(view);
    }

    public final void startUpdates() {
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

    public final void stopUpdates() {
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

        private final int[] bands;
        private final int[] levels;
        private int channels;
        private AtomicBoolean scheduled;

        UpdateUiTask() {
            this.bands = new int[SpectrumAnalyzerView.MAX_BANDS];
            this.levels = new int[SpectrumAnalyzerView.MAX_BANDS];
            this.scheduled = new AtomicBoolean();
        }

        final void execute(Visualizer src) throws Exception {
            this.channels = src.getSpectrum(bands, levels);
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
            if (view != null && view.update(channels, bands, levels)) {
                channels = 0;
                if (scheduled.compareAndSet(false, true)) {
                    handler.postDelayed(this, PERIOD);
                }
            }
        }
    }
}
