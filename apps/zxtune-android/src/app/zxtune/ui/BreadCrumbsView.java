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

import java.util.List;

import android.content.Context;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import app.zxtune.R;

public class BreadCrumbsView extends HorizontalScrollView {

  public interface OnUriSelectionListener {

    public void onUriSelection(Uri uri);
  }

  public static class StubUriSelectionListener implements OnUriSelectionListener {
    @Override
    public void onUriSelection(Uri uri) {}
  }
  
  private final static String ROOT = "/";
  private ViewGroup container;
  private OnUriSelectionListener listener;

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

  private final void setupView() {
    container = new LinearLayout(getContext());
    addView(container);
    listener = new StubUriSelectionListener();
  }

  public void setUri(Uri uri) {
    if (uri.equals(Uri.EMPTY)) {
      setButtonsCount(0);
    } else {
      final Uri.Builder subUri = uri.buildUpon().path(ROOT);
      final String root = getRoot(uri);
      final List<String> elements = uri.getPathSegments();
      fillButtons(subUri, root, elements);
      scrollToEnd();
    }
  }

  public void setOnUriSelectionListener(OnUriSelectionListener listener) {
    this.listener = listener != null ? listener : new StubUriSelectionListener();
  }

  private final void fillButtons(Uri.Builder uri, String root, List<String> texts) {
    setButtonsCount(texts.size() + 1);
    enableButton(0, root, uri.build());
    for (int idx = 0; idx != texts.size(); ++idx) {
      final String text = texts.get(idx);
      final Uri point = uri.appendPath(text).build();
      enableButton(idx + 1, text, point);
    }
  }

  private final void enableButton(int idx, String text, Uri point) {
    final Button button = (Button) container.getChildAt(idx);
    button.setVisibility(VISIBLE);
    button.setText(text);
    button.setTag(point);
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
    final LayoutInflater inflater = (LayoutInflater)getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    for (int idx = startId; idx != endId; ++idx) {
      final Button button = (Button)inflater.inflate(R.layout.button, container, false);
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

  private final String getRoot(Uri uri) {
    final String root = uri.getHost();
    return root != null ? root : ROOT;
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
}
