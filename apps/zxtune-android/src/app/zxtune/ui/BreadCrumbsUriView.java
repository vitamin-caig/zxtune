/*
 * @file
 * 
 * @brief Uri view in bread crumbs-style
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

public class BreadCrumbsUriView extends HorizontalScrollView {

  interface OnUriSelectionListener {

    public void onUriSelection(Uri uri);
  }

  private ViewGroup container;
  private OnUriSelectionListener listener;

  public BreadCrumbsUriView(Context context) {
    super(context);

    setupView();
  }

  public BreadCrumbsUriView(Context context, AttributeSet attr) {
    super(context, attr);

    setupView();
  }

  public BreadCrumbsUriView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);

    setupView();
  }

  public void setUri(Uri uri) {
    final Uri.Builder subUri = uri.buildUpon();
    subUri.path("");
    final List<String> elements = getPathElements(uri);
    fillButtons(subUri, elements);
    scrollToEnd();
  }

  public void setOnUriSelectionListener(OnUriSelectionListener listener) {
    this.listener = listener != null ? listener : new StubUriSelectionListener();
  }

  private final void setupView() {
    container = new LinearLayout(getContext());
    addView(container);
    listener = new StubUriSelectionListener();
  }

  private final void fillButtons(Uri.Builder uri, List<String> texts) {
    setButtonsCount(texts.size());
    for (int idx = 0; idx != texts.size(); ++idx) {
      final String text = texts.get(idx);
      final Uri point = uri.appendPath(text).build();
      final Button button = (Button) container.getChildAt(idx);
      button.setVisibility(VISIBLE);
      button.setText(text);
      button.setTag(point);
    }
  }

  private final void setButtonsCount(int count) {
    final int current = container.getChildCount();
    if (current < count) {
      createNewButtons(current, count);
    } else if (current > count) {
      hideButtons(count, current);
    }
  }

  private final void createNewButtons(int startId, int endId) {
    for (int idx = startId; idx != endId; ++idx) {
      final Button button = new Button(getContext());
      button.setOnClickListener(new OnClickListener() {
        @Override
        public void onClick(View v) {
          final Uri uri = (Uri) v.getTag();
          listener.onUriSelection(uri);
        }
      });
      container.addView(button, idx);
    }
  }

  private final void hideButtons(int startId, int endId) {
    for (int idx = startId; idx != endId; ++idx) {
      container.getChildAt(idx).setVisibility(GONE);
    }
  }

  private final List<String> getPathElements(Uri uri) {
    final List<String> elements = new ArrayList<String>();
    elements.add("/");
    elements.addAll(uri.getPathSegments());
    return elements;
  }

  private final void scrollToEnd() {
    // workaround for layout timing issues
    post(new Runnable() {
      @Override
      public void run() {
        smoothScrollTo(Integer.MAX_VALUE, 0);
      }
    });
  }

  private class StubUriSelectionListener implements OnUriSelectionListener {
    @Override
    public void onUriSelection(Uri uri) {}
  }
}
