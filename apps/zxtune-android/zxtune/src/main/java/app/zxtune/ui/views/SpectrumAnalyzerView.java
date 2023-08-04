/**
 * @file
 * @brief Visualizer view component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui.views;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.Build;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.ColorInt;
import androidx.annotation.Nullable;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.stubs.VisualizerStub;

public class SpectrumAnalyzerView extends SurfaceView implements SurfaceHolder.Callback {

  private static final int MAX_LEVEL = 100;
  private static final int BAR_PADDING = 1;
  private static final int FALL_SPEED = 2;
  private static final int UPDATE_FPS = 30;

  private final SpectrumVisualizer visualizer;
  @Nullable
  private Rect visibleRect;
  @Nullable
  private Visualizer source = VisualizerStub.instance();
  private boolean isUpdating;
  @Nullable
  private RenderThread thread;

  public SpectrumAnalyzerView(Context context) {
    this(context, null);
  }

  public SpectrumAnalyzerView(Context context, @Nullable AttributeSet attrs) {
    this(context, attrs, 0);
  }

  public SpectrumAnalyzerView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    final TypedArray a = context.getTheme().obtainStyledAttributes(attrs, R.styleable.AnalyzerView, defStyleAttr, 0);
    try {
      visualizer = createSpectrumVisualizer(a);
      init();
    } finally {
      a.recycle();
    }
  }

  private SpectrumVisualizer createSpectrumVisualizer(TypedArray a) {
    final int barColor = a.getColor(R.styleable.AnalyzerView_spectrumBarsColor, 0xffffff);
    final int maxBarsCount = a.getInteger(R.styleable.AnalyzerView_spectrumMaxBarsCount, 48);
    final int minBarWidth = a.getDimensionPixelSize(R.styleable.AnalyzerView_spectrumMinBarWidth, 3);
    return new SpectrumVisualizer(barColor, maxBarsCount, minBarWidth);
  }

  private void init() {
    setZOrderOnTop(true);
    setLayerType(LAYER_TYPE_HARDWARE, null);
    getHolder().setFormat(PixelFormat.TRANSPARENT);
    visibleRect = new Rect();
    getHolder().addCallback(this);
  }

  public final void setSource(@Nullable Visualizer source) {
    this.source = source != null ? source : VisualizerStub.instance();
  }

  public final void setIsUpdating(boolean updating) {
    if (isUpdating != updating) {
      isUpdating = updating;
      if (thread != null) {
        thread.updateState();
      }
    }
  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    final int padLeft = getPaddingLeft();
    final int padRight = getPaddingRight();
    final int padTop = getPaddingTop();
    final int padBottom = getPaddingBottom();
    final int padHorizontal = padLeft + padRight;
    final int padVertical = padTop + padBottom;
    if (padHorizontal < w && padVertical < h) {
      visibleRect.left = padLeft;
      visibleRect.right = w - padRight;
      visibleRect.top = padTop;
      visibleRect.bottom = h - padBottom;
    } else {
      visibleRect.set(0, 0, w, h);
    }
    visualizer.sizeChanged();
  }

  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    thread.setHolder(holder);
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    thread.setHolder(null);
  }

  @Override
  public void onAttachedToWindow() {
    super.onAttachedToWindow();
    thread = new RenderThread();
  }

  @Override
  public void onDetachedFromWindow() {
    super.onDetachedFromWindow();
    thread.interrupt();
    thread = null;
  }

  private class RenderThread extends Thread {

    @Nullable
    private SurfaceHolder holder;
    private long scheduledFrameTime;

    RenderThread() {
      super("Visualizer");
      start();
    }

    synchronized final void setHolder(@Nullable SurfaceHolder holder) {
      if (this.holder != holder) {
        this.holder = holder;
        notify();
      }
    }

    synchronized final void updateState() {
      notify();
    }

    @Override
    public void run() {
      try {
        final byte[] levels = new byte[visualizer.maxBarsCount];
        for (; ; ) {
          if (isUpdating) {
            final int channels = source.getSpectrum(levels);
            if (channels != 0) {
              visualizer.update(channels, levels);
            } else {
              visualizer.update();
            }
            synchronizedDraw();
          } else if (visualizer.update()) {
            synchronizedDraw();
          } else {
            synchronized(this) {
              this.wait();
            }
          }
        }
      } catch (InterruptedException e) {
        // do nothing
      } catch (Exception e) {
        Log.w(getName(), e, "Failed!");
      }
    }

    private void synchronizedDraw() throws InterruptedException {
      sync();
      draw();
    }

    private void sync() {
      final long MILLIS_PER_FRAME = 1000 / UPDATE_FPS;
      final long now = SystemClock.elapsedRealtime();
      if (now <= scheduledFrameTime) {
        SystemClock.sleep(scheduledFrameTime - now);
      } else {
        //resync or maximal possible framerate
        scheduledFrameTime = now;
        Log.d("Visualizer", "resync!!!");
      }
      scheduledFrameTime += MILLIS_PER_FRAME;
    }

    private synchronized void draw() throws InterruptedException {
      final Canvas canvas = lockCanvas();
      if (canvas != null) {
        try {
          canvas.drawColor(0, android.graphics.PorterDuff.Mode.CLEAR);
          visualizer.draw(canvas);
        } finally {
          holder.unlockCanvasAndPost(canvas);
        }
      }
    }

    private Canvas lockCanvas() throws InterruptedException {
      while (holder == null) {
        wait();
      }
      return Build.VERSION.SDK_INT >= 26 ? holder.lockHardwareCanvas() : holder.lockCanvas();
    }
  }

  private final class SpectrumVisualizer {

    private final int maxBarsCount;
    private final int minBarWidth;
    private final Rect barRect;
    private final Paint paint;
    private int barWidth;
    private int[] values;

    SpectrumVisualizer(@ColorInt int barColor, int maxBarsCount, int minBarWidth) {
      this.maxBarsCount = maxBarsCount;
      this.minBarWidth = minBarWidth;
      this.barRect = new Rect();
      this.paint = new Paint();
      this.paint.setColor(barColor);
      this.barWidth = minBarWidth;
      this.values = new int[1];
    }

    void sizeChanged() {
      barRect.bottom = visibleRect.bottom;
      final int width = visibleRect.width();
      barWidth = Math.max(width / maxBarsCount, minBarWidth);
      final int bars = Math.max(width / barWidth, 1);
      values = new int[bars];
    }

    void draw(Canvas canvas) {
      barRect.left = visibleRect.left;
      barRect.right = barRect.left + barWidth - BAR_PADDING;
      final int height = visibleRect.height();
      for (int level : values) {
        if (level != 0) {
          barRect.top = visibleRect.top + height - level * height / MAX_LEVEL;
          canvas.drawRect(barRect, paint);
        }
        barRect.offset(barWidth, 0);
      }
    }

    void update(int channels, byte[] levels) {
      for (int band = 0, lim = Math.min(channels, values.length); band < lim; ++band) {
        values[band] = Math.max(values[band] - FALL_SPEED, levels[band]);
      }
    }

    boolean update() {
      boolean result = false;
      for (int band = 0; band < values.length; ++band) {
        if (values[band] >= FALL_SPEED) {
          values[band] -= FALL_SPEED;
          result = true;
        } else {
          values[band] = 0;
        }
      }
      return result;
    }
  }
}
