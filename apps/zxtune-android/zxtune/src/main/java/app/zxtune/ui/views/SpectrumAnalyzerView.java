/**
 *
 * @file
 *
 * @brief Visualizer view component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;

public class SpectrumAnalyzerView extends View {

  public static final int MAX_BANDS = 96;
  public static final int MAX_LEVEL = 100;
  private static final int MIN_BAR_WIDTH = 3;
  private static final int BAR_PADDING = 1;
  private static final int FALL_SPEED = 10;

  private Rect visibleRect;
  
  private SpectrumVisualizer visualizer;
  
  public SpectrumAnalyzerView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    init();
  }
  
  public SpectrumAnalyzerView(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }
  
  public SpectrumAnalyzerView(Context context) {
    super(context);
    init();
  }

  public final boolean update(int channels, byte[] levels) {
    return visualizer.update(channels, levels);
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
    visibleRect = new Rect();
    visualizer = new SpectrumVisualizer();
    setWillNotDraw(false);
  }
  
  private void fillVisibleRect(int w, int h) {
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
  
  private final class SpectrumVisualizer {
    
    private final Rect barRect;
    private final Paint paint;
    private int barWidth;
    private int[] values;
    private boolean[] changes;
    private int lowerChange;
    private int upperChange;

    SpectrumVisualizer() {
      this.barRect = new Rect();
      this.paint = new Paint();
      this.paint.setColor(getResources().getColor(android.R.color.primary_text_dark));
      this.barWidth = MIN_BAR_WIDTH;
      this.values = new int[1];
      this.changes = new boolean[1];
    }
    
    final void sizeChanged() {
      barRect.bottom = visibleRect.bottom;
      final int width = visibleRect.width();
      barWidth = Math.max(width / MAX_BANDS, MIN_BAR_WIDTH);
      final int bars = Math.max(width / barWidth, 1);
      values = new int[bars];
      changes = new boolean[bars];
      lowerChange = 0;
      upperChange = bars - 1;
    }

    final boolean update(int channels, byte[] levels) {
      updateValues(channels, levels);
      if (lowerChange != upperChange) {
        final int updateLeft = visibleRect.left + barWidth * lowerChange;
        final int updateRight = visibleRect.left + barWidth * upperChange;
        invalidate(updateLeft, visibleRect.top, updateRight, visibleRect.bottom);
        return true;
      } else {
        return false;
      }
    }

    final void draw(Canvas canvas) {
      barRect.left = visibleRect.left + barWidth * lowerChange;
      barRect.right = barRect.left + barWidth - BAR_PADDING;
      final int height = visibleRect.height();
      for (int band = lowerChange; band < upperChange; ++band) {
        if (changes[band]) {
          barRect.top = visibleRect.top + height - values[band];
          canvas.drawRect(barRect, paint);
          changes[band] = false;
        }
        barRect.offset(barWidth, 0);
      }
    }

    private void updateValues(int channels, byte[] levels) {
      fallBars();
      final int height = visibleRect.height();
      for (int band = 0; band != channels; ++band) {
        final byte level = levels[band];
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
}
