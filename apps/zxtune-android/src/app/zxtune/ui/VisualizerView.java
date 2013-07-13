/*
 * @file
 * @brief VisualizerView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;
import app.zxtune.R;
import app.zxtune.playback.Visualizer;
import app.zxtune.playback.VisualizerStub;

public class VisualizerView extends View {
  
  private Visualizer source;
  private Handler timer;
  private Runnable updateTask;

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
  
  public void setSource(Visualizer source) {
    if (source == null) {
      this.source = VisualizerStub.instance();
      timer.removeCallbacks(updateTask);
    } else {
      this.source = source;
      timer.post(updateTask);
    }
  }
  
  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    fillVisibleRect(w, h);
  }
  
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    visualizer.draw(canvas);
  }
  
  private void init() {
    source = VisualizerStub.instance();
    timer = new Handler();
    updateTask = new UpdateViewTask();
    visibleRect = new Rect();
    visualizer = new SpectrumVisualizer();
    setWillNotDraw(false);
  }
  
  private void fillVisibleRect(int w, int h) {
    visibleRect.left = getPaddingLeft();
    visibleRect.right = w - visibleRect.left - getPaddingRight();
    visibleRect.top = getPaddingTop();
    visibleRect.bottom = h - visibleRect.top - getPaddingBottom();
    visualizer.sizeChanged();
  }
  
  private final class SpectrumVisualizer {
    
    private static final int MAX_LEVEL = 100;
    private static final int BAR_WIDTH = 4;
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
      this.paint.setColor(getResources().getColor(R.color.primary));
      this.bands = new int[16];
      this.levels = new int[this.bands.length];
    }
    
    public void sizeChanged() {
      barRect.bottom = visibleRect.bottom;
      values = new int[visibleRect.width() / BAR_WIDTH];
      changes = new boolean[values.length];
    }

    public void update() {
      final int channels = source.getSpectrum(bands, levels);
      updateValues(channels);
      final int updateLeft = visibleRect.left + BAR_WIDTH * lowerChange;
      final int updateRight = visibleRect.left + BAR_WIDTH * (upperChange + 1);
      postInvalidate(updateLeft, visibleRect.top, updateRight, visibleRect.bottom);
    }

    public void draw(Canvas canvas) {
      barRect.left = visibleRect.left + BAR_WIDTH * lowerChange;
      barRect.right = barRect.left + BAR_WIDTH - BAR_PADDING;
      final int height = visibleRect.height();
      for (int band = lowerChange; band <= upperChange; ++band) {
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
      for (lowerChange = 0; lowerChange != changes.length && !changes[lowerChange]; ++lowerChange);
      for (upperChange = changes.length - 1; upperChange > lowerChange && !changes[upperChange]; --upperChange);
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
      visualizer.update();
      timer.postDelayed(this, 100);
    }
  }
}
