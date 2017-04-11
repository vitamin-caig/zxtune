/**
 *
 * @file
 *
 * @brief Visualizer view component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;

import app.zxtune.Log;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.VisualizerStub;

public class VisualizerView extends View {
  
  private static final String TAG = VisualizerView.class.getName();
  private Visualizer source;
  //use dedicated looper for smooth visualizer
  private Handler looper;
  private UpdateViewTask updateTask;

  private Rect visibleRect;
  
  private SpectrumVisualizer visualizer;
  
  public VisualizerView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    init();
  }
  
  public VisualizerView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }
  
  public VisualizerView(Context context) {
    super(context);
    init();
  }
  
  final synchronized void setSource(Visualizer source) {
    this.source = source;
  }
  
  @Override
  public void setEnabled(boolean enabled) {
    super.setEnabled(enabled);
    updateTask.stop();
    if (enabled) {
      updateTask.run();
    }
  }
  
  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    fillVisibleRect(w, h);
  }
  
  @Override
  protected synchronized void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    visualizer.draw(canvas);
  }
  
  private void init() {
    source = VisualizerStub.instance();
    looper = new Handler();
    updateTask = new UpdateViewTask();
    visibleRect = new Rect();
    visualizer = new SpectrumVisualizer();
    setWillNotDraw(false);
  }
  
  private synchronized void fillVisibleRect(int w, int h) {
    final int padLeft = getPaddingLeft();
    final int padRight = getPaddingRight();
    final int padTop = getPaddingTop();
    final int padBottom = getPaddingBottom();
    final int padHorizontal = padLeft + padRight;
    final int padVertical = padTop + padBottom;
    if (padHorizontal < w || padVertical < h) {
      visibleRect.left = padLeft;
      visibleRect.right = w - padHorizontal;
      visibleRect.top = padTop;
      visibleRect.bottom = h - padVertical;
    } else {
      visibleRect.set(0, 0, w, h);
    }
    visualizer.sizeChanged();
  }
  
  private final class SpectrumVisualizer {
    
    private static final int MAX_BANDS = 32;
    private static final int MAX_LEVEL = 100;
    private static final int BAR_WIDTH = 3;
    private static final int BAR_PADDING = 1;
    private static final int FALL_SPEED = 10;

    private final Rect barRect;
    private final Paint paint;
    private final int[] bands;
    private final int[] levels;
    private int[] values;
    private boolean[] changes;
    private int lowerChange;
    private int upperChange;
    
    public SpectrumVisualizer() {
      this.barRect = new Rect();
      this.paint = new Paint();
      this.paint.setColor(getResources().getColor(android.R.color.primary_text_dark));
      this.bands = new int[MAX_BANDS];
      this.levels = new int[MAX_BANDS];
      this.values = new int[1];
      this.changes = new boolean[1];
    }
    
    final void sizeChanged() {
      barRect.bottom = visibleRect.bottom;
      final int bars = visibleRect.width() / BAR_WIDTH;
      values = new int[bars];
      changes = new boolean[bars];
      lowerChange = 0;
      upperChange = bars - 1;
    }

    final boolean update() throws Exception {
      final int channels = source.getSpectrum(bands, levels);
      updateValues(channels);
      if (lowerChange != upperChange) {
        final int updateLeft = visibleRect.left + BAR_WIDTH * lowerChange;
        final int updateRight = visibleRect.left + BAR_WIDTH * upperChange;
        invalidate(updateLeft, visibleRect.top, updateRight, visibleRect.bottom);
        return true;
      }
      return false;
    }

    final void draw(Canvas canvas) {
      barRect.left = visibleRect.left + BAR_WIDTH * lowerChange;
      barRect.right = barRect.left + BAR_WIDTH - BAR_PADDING;
      final int height = visibleRect.height();
      for (int band = lowerChange; band < upperChange; ++band) {
        if (changes[band]) {
          barRect.top = visibleRect.top + height - values[band];
          canvas.drawRect(barRect, paint);
          changes[band] = false;
        }
        barRect.offset(BAR_WIDTH, 0);
      }
    }

    private void updateValues(int channels) {
      fallBars();
      final int height = visibleRect.height();
      for (int idx = 0; idx != channels; ++idx) {
        final int band = bands[idx];
        final int level = levels[idx];
        if (level != 0 && band < values.length) {
          final int newLvl = level * height / MAX_LEVEL;
          if (newLvl > values[band]) {
            values[band] = newLvl;
            changes[band] = true;
          }
        }
      }
      lowerChange = 0;
      while (lowerChange != changes.length && !changes[lowerChange]) {
        ++lowerChange;
      }
      upperChange = changes.length;
      while (upperChange > lowerChange && !changes[upperChange - 1]) {
        --upperChange;
      }
    }

    private void fallBars() {
      final int fall = visibleRect.height() * FALL_SPEED / MAX_LEVEL;
      for (int i = 0; i != values.length; ++i) {
        if (values[i] != 0) {
          values[i] = Math.max(0, values[i] - fall);
          changes[i] = true;
        }
      }
    }
  }
  
  private final class UpdateViewTask implements Runnable {
    
    @Override
    public void run() {
      try {
        update();
        looper.postDelayed(this, 100);
      } catch (Exception e) {
        Log.w(TAG, e, "UpdateViewTask");
      }
    }
    
    private synchronized boolean update() throws Exception {
      return visualizer.update();
    }

    final void stop() {
      looper.removeCallbacks(this);
    }
  }
}
