/*
 * @file
 * @brief SpectrumAnalyzerView class
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.Arrays;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;

public class SpectrumAnalyzerView extends View {
  
  private static final int MAX_BANDS = 12 * 8;
  private static final int MAX_LEVEL = 100;
  private static final int BAR_WIDTH = 4;
  private static final int BAR_PADDING = 1;
  private static final int BAR_COLOR = 0xffffff;
  private static final int FALLING = 10;
  private static final int WIDTH = MAX_BANDS * BAR_WIDTH;
  private static final int HEIGHT = MAX_LEVEL;
  private final int[] bitmap = new int[WIDTH * HEIGHT];
  private final Rect visibleRect = new Rect();

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
    fallBitmap();
    for (int sp : spectrum) {
      final int band = sp & 0xff;
      final int level = sp >> 8;
      if (level != 0 && band < MAX_BANDS) {
        drawBar(band, level);
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
    
    final int width = Math.min(visibleRect.width(), WIDTH);
    final int height = Math.min(visibleRect.height(), HEIGHT);
    canvas.drawBitmap(bitmap, bitmap.length - WIDTH, -WIDTH, 
      visibleRect.left, visibleRect.top, width, height, 
      false, null);
  }
  
  private void init() {
    fillVisibleRect(getWidth(), getHeight());
    setWillNotDraw(false);
  }
  
  private void fillVisibleRect(int w, int h) {
    visibleRect.left = getPaddingLeft();
    visibleRect.right = w - visibleRect.left - getPaddingRight();
    visibleRect.top = getPaddingTop();
    visibleRect.bottom = h - visibleRect.top - getPaddingBottom();
  }
  
  private void fallBitmap() {
    final int delta = WIDTH * FALLING;
    System.arraycopy(bitmap, delta, bitmap, 0, bitmap.length - delta);
    Arrays.fill(bitmap, bitmap.length - delta, bitmap.length, 0);
  }
  
  private void drawBar(int band, int level) {
    int offset = band * BAR_WIDTH;
    for (int h = 0; h != level; ++h) {
      Arrays.fill(bitmap, offset, offset + BAR_WIDTH - BAR_PADDING, BAR_COLOR);
      offset += WIDTH;
    }
  }
}
