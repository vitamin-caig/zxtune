/*
 * @file
 * @brief SpectrumAnalyzerView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;
import app.zxtune.R;

public class SpectrumAnalyzerView extends View {
  
  private static final int MAX_LEVEL = 100;
  private static final int BAR_WIDTH = 4;
  private static final int BAR_PADDING = 1;
  private static final int FALL_SPEED = 10;

  private final Rect visibleRect = new Rect();
  private final Rect barRect = new Rect();
  private final Rect updateRect = new Rect();
  private Paint paint;
  private int[] levels;
  private boolean[] changes;
  private int lowerChange;
  private int upperChange;

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
  
  public void update(int[] spectrum) {
    fallBars();
    final int height = visibleRect.height();
    for (int sp : spectrum) {
      final int band = sp & 0xff;
      final int level = sp >> 8;
      if (level != 0 && band < levels.length) {
        final int newLvl = level * height / MAX_LEVEL;
        if (newLvl > levels[band]) {
          levels[band] = newLvl;
          changes[band] = true;
        }
      }
    }
    for (lowerChange = 0; lowerChange != changes.length && !changes[lowerChange]; ++lowerChange);
    for (upperChange = changes.length - 1; upperChange > lowerChange && !changes[upperChange]; --upperChange);
    updateRect.left = BAR_WIDTH * lowerChange;
    updateRect.right = BAR_WIDTH * (upperChange + 1);
    invalidate(updateRect);
  }
  
  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    fillVisibleRect(w, h);
  }
  
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    
    barRect.left = visibleRect.left + BAR_WIDTH * lowerChange;
    barRect.right = barRect.left + BAR_WIDTH - BAR_PADDING;
    final int height = visibleRect.height();
    for (int band = lowerChange; band <= upperChange; ++band) {
      if (changes[band]) {
        barRect.top = visibleRect.top + height - levels[band];
        canvas.drawRect(barRect, paint);
        changes[band] = false;
      }
      barRect.offset(BAR_WIDTH, 0);
    }
  }
  
  private void init() {
    fillVisibleRect(getWidth(), getHeight());
    setWillNotDraw(false);
    paint = new Paint();
    paint.setColor(getResources().getColor(R.color.primary));
  }
  
  private void fillVisibleRect(int w, int h) {
    visibleRect.left = getPaddingLeft();
    visibleRect.right = w - visibleRect.left - getPaddingRight();
    visibleRect.top = getPaddingTop();
    visibleRect.bottom = h - visibleRect.top - getPaddingBottom();
    barRect.bottom = visibleRect.bottom;
    updateRect.top = visibleRect.top;
    updateRect.bottom = visibleRect.bottom;
    levels = new int[w / BAR_WIDTH];
    changes = new boolean[levels.length];
  }
  
  private void fallBars() {
    final int fall = visibleRect.height() * FALL_SPEED / MAX_LEVEL;
    for (int i = 0; i != levels.length; ++i) {
      if (levels[i] != 0) {
        levels[i] = Math.max(0, levels[i] - fall);
        changes[i] = true;
      }
    }
  }
}
