/**
 *
 * @file
 *
 * @brief Breadcrumb-style navigation widget
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.browser;

import java.util.ArrayList;
import java.util.Collections;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;

public class BreadCrumbsView extends HorizontalScrollView {

  public interface DirSelectionListener {

    public void onDirSelection(VfsDir dir);
  }

  public static class StubDirSelectionListener implements DirSelectionListener {
    @Override
    public void onDirSelection(VfsDir dir) {}
  }
  
  private ViewGroup container;
  private DirSelectionListener listener;

  public BreadCrumbsView(Context context) {
    super(context);
    setupView();
  }

  public BreadCrumbsView(Context context, AttributeSet attr) {
    super(context, attr);
    setupView();
  }

  public BreadCrumbsView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    setupView();
  }

  private void setupView() {
    container = new LinearLayout(getContext());
    addView(container);
    listener = new StubDirSelectionListener();
  }

  public final void setDir(VfsDir dir) {
    if (dir == null || dir == Vfs.getRoot()) {
      hideButtons(0, container.getChildCount());
    } else {
      final ArrayList<VfsDir> elems = new ArrayList<VfsDir>();
      while (dir != null) {
        elems.add(dir);
        dir = (VfsDir) dir.getParent();
      }
      Collections.reverse(elems);
      fillButtons(elems);
      scrollToEnd();
    }
  }

  public final void setDirSelectionListener(DirSelectionListener listener) {
    this.listener = listener != null ? listener : new StubDirSelectionListener();
  }

  private void fillButtons(ArrayList<VfsDir> dirs) {
    int idx = 0;
    for (int lim = dirs.size(); idx != lim; ++idx) {
      showButton(idx, dirs.get(idx));
    }
    hideButtons(idx, container.getChildCount());
  }

  private void showButton(int idx, VfsDir dir) {
    final View but = getButton(idx, dir);
    but.setVisibility(VISIBLE);
    but.setTag(dir);
  }
  
  private View getButton(int idx, VfsDir dir) {
    final View but = container.getChildAt(idx);
    return but == null
      ? createButton(idx, dir)
      : updateButton((Button) but, dir);
  }
  
  private View createButton(int idx, VfsDir dir) {
    final View res = createButton(dir);
    res.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        listener.onDirSelection((VfsDir) v.getTag());
      }
    });
    container.addView(res, idx);
    return res;
  }
  
  private View createButton(VfsDir dir) {
    final LayoutInflater inflater = LayoutInflater.from(getContext());
    return updateButton((Button) inflater.inflate(R.layout.button, container, false), dir);
  }
  
  private View updateButton(Button but, VfsDir dir) {
    final Drawable[] currentImages = but.getCompoundDrawables();
    final Drawable icon = getIcon(dir);
    if (icon != currentImages[0]) {
      but.setCompoundDrawablesWithIntrinsicBounds(icon, currentImages[1], currentImages[2], currentImages[3]);
      if (icon != null) {
        but.setText(null);
      }
    }
    if (icon == null) {
      but.setText(dir.getName());
    }
    return but;
  }

  private Drawable getIcon(VfsDir dir) {
    final Object icon = dir.getExtension(VfsExtensions.ICON_RESOURCE);
    return icon != null
            ? getResources().getDrawable((Integer) icon)
            : null;
  }
  
  private void hideButtons(int startId, int endId) {
    for (int idx = startId; idx < endId; ++idx) {
      final View view = container.getChildAt(idx);
      view.setVisibility(GONE);
      view.setTag(null);
    }
  }

  private void scrollToEnd() {
    // workaround for layout timing issues
    post(new Runnable() {
      @Override
      public void run() {
        smoothScrollTo(Integer.MAX_VALUE, 0);
      }
    });
  }
}
