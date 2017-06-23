/**
 *
 * @file
 *
 * @brief RelativeLayout implementation with Checkable interface and functionality support
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Checkable;
import android.widget.RelativeLayout;

import java.util.LinkedList;

public class CheckableRelativeLayout extends RelativeLayout implements Checkable {

  private static final int[] CHECKED_STATE_SET = {android.R.attr.state_checked}; 
  private boolean isChecked;
  private LinkedList<Checkable> delegates;
  private StateListDrawable background;
  
  public CheckableRelativeLayout(Context context) {
    super(context);
  }

  public CheckableRelativeLayout(Context context, AttributeSet attrs) {
    super(context, attrs);
  }
  
  /*
   * Delegate checking to children or self if there are no checkable children at all
   */
  @Override
  public void onFinishInflate() {
    super.onFinishInflate();
    final LinkedList<Checkable> delegates = new LinkedList<>();
    addCheckables(this, delegates);
    if (!delegates.isEmpty()) {
      this.delegates = delegates;
    } else {
      final Drawable background = getBackground();
      if (background instanceof StateListDrawable) {
        this.background = (StateListDrawable) background;
      }
    }
  }
  
  private void addCheckables(ViewGroup v, LinkedList<Checkable> delegates) {
    final int count = v.getChildCount();
    for (int i = 0; i != count; ++i) {
      addCheckable(v.getChildAt(i), delegates);
    }
  }
  
  private void addCheckable(View v, LinkedList<Checkable> delegates) {
    if (v instanceof Checkable) {
      delegates.add((Checkable) v);
    } else if (v instanceof ViewGroup) {
      addCheckables((ViewGroup) v, delegates);
    }
  }

  @Override
  public boolean isChecked() {
    return isChecked;
  }

  @Override
  public void setChecked(boolean checked) {
    isChecked = checked;
    if (delegates != null) {
      for (Checkable c : delegates) {
        c.setChecked(checked);
      }
    } else if (background != null) {
      updateSelf();
    }
  }

  @Override
  public void toggle() {
    isChecked = !isChecked;
    if (delegates != null) {
      for (Checkable c : delegates) {
        c.toggle();
      }
    } else if (background != null) {
      updateSelf();
    }
  }
  
  @Override
  protected int[] onCreateDrawableState(int extraSpace) {
    final int[] state = super.onCreateDrawableState(extraSpace + 1);
    if (isChecked) {
      mergeDrawableStates(state, CHECKED_STATE_SET);
    }
    return state;
  }

  private void updateSelf() {
    background.setState(isChecked ? CHECKED_STATE_SET : EMPTY_STATE_SET);
  }
}
