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
import app.zxtune.R;

public class SpectrumAnalyzerView extends View {
  
  private static final int MAX_LEVEL = 100;
  private static final int BAR_WIDTH = 4;
  private static final int BAR_PADDING = 1;
  private static final int FALL_SPEED = 10;
  private final Rect visibleRect = new Rect();
  private int[] bitmap;
  private int barColor;

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
    final int height = visibleRect.height();
    final int maxBands = visibleRect.width() / BAR_WIDTH;
    for (int sp : spectrum) {
      final int band = sp & 0xff;
      final int level = sp >> 8;
      if (level != 0 && band < maxBands) {
        drawBar(band, level * height / MAX_LEVEL);
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
    
    final int width = visibleRect.width();
    final int height = visibleRect.height();
    canvas.drawBitmap(bitmap, bitmap.length - width, -width, 
      visibleRect.left, visibleRect.top, width, height, 
      false, null);
  }
  
  private void init() {
    fillVisibleRect(getWidth(), getHeight());
    setWillNotDraw(false);
    barColor = getResources().getColor(R.color.primary);
  }
  
  private void fillVisibleRect(int w, int h) {
    visibleRect.left = getPaddingLeft();
    visibleRect.right = w - visibleRect.left - getPaddingRight();
    visibleRect.top = getPaddingTop();
    visibleRect.bottom = h - visibleRect.top - getPaddingBottom();
    bitmap = new int[w * h];
  }
  
  private void fallBitmap() {
    final int fall = visibleRect.height() * FALL_SPEED / MAX_LEVEL;
    final int delta = visibleRect.width() * fall;
    System.arraycopy(bitmap, delta, bitmap, 0, bitmap.length - delta);
    Arrays.fill(bitmap, bitmap.length - delta, bitmap.length, 0);
  }
  
  private void drawBar(int band, int height) {
    int offset = band * BAR_WIDTH;
    final int delta = visibleRect.width();
    for (int h = 0; h != height; ++h) {
      Arrays.fill(bitmap, offset, offset + BAR_WIDTH - BAR_PADDING, barColor);
      offset += delta;
    }
  }
}
