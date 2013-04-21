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

public class SpectrumAnalyzerView extends View {
  
  private static final int BAR_WIDTH = 4;
  private static final int MAX_BANDS = 12 * 8;
  private static final int FALLING = 10;
  private final int[] levels = new int[MAX_BANDS];
  private final Rect visibleRect = new Rect();
  private final Paint outerPaint = new Paint();
  private final Rect barRect = new Rect();
  private int visibleHeight;
  private int visibleBands;

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
  
  public int getMaxBands() {
    return MAX_BANDS; 
  }
  
  public void update(int[] spectrum) {
    final int totalBands = Math.min(spectrum.length, levels.length);
    int minChanges = totalBands, maxChanges = 0; 
    for (int band = 0; band != totalBands; ++band) {
      final int oldVal = levels[band];
      final int newVal = Math.max(levels[band] - FALLING, spectrum[band]);
      if (newVal != oldVal) {
        levels[band] = newVal;
        minChanges = Math.min(minChanges, band);
        maxChanges = Math.max(maxChanges, band);
      }
    }
    this.invalidate(visibleRect.left + minChanges * BAR_WIDTH, visibleRect.top,
      visibleRect.left + (maxChanges + 1) * BAR_WIDTH, visibleRect.bottom);
  }
  
  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    fillVisibleRect(w, h);
  }
  
  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);
    
    barRect.left = visibleRect.left;
    barRect.right = BAR_WIDTH - 1;
    barRect.bottom = visibleRect.bottom;
    for (int band = 0; band != visibleBands; ++band) {
      assert levels[band] <= 100;
      barRect.top = barRect.bottom - levels[band] * visibleHeight / 100;
      canvas.drawRect(barRect, outerPaint);
      barRect.offset(BAR_WIDTH, 0);
    }
  }
  
  private void init() {
    fillVisibleRect(getWidth(), getHeight());
    outerPaint.setColor(0xffffffff);
    outerPaint.setStyle(Paint.Style.FILL_AND_STROKE);
    setWillNotDraw(false);
  }
  
  private void fillVisibleRect(int w, int h) {
    visibleRect.left = getPaddingLeft();
    visibleRect.right = w - visibleRect.left - getPaddingRight();
    visibleRect.top = getPaddingTop();
    visibleRect.bottom = h - visibleRect.top - getPaddingBottom();
    visibleHeight = visibleRect.height();
    visibleBands = Math.min(visibleRect.width() / BAR_WIDTH, levels.length);
  }
}
