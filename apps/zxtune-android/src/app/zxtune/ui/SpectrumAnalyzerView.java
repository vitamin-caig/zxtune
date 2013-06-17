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
  private Paint paint;
  private int[] levels;
  private boolean[] changes;

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
    invalidate(visibleRect);
  }
  
  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    fillVisibleRect(w, h);
  }
  
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    
    barRect.left = visibleRect.left;
    barRect.right = barRect.left + BAR_WIDTH;
    final int height = visibleRect.height();
    for (int band = 0; band != levels.length; ++band) {
      if (changes[band]) {
        barRect.top = visibleRect.top + height - levels[band];
        canvas.drawRect(barRect, paint);
        changes[band] = false;
      }
      barRect.offset(BAR_WIDTH + BAR_PADDING, 0);
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
