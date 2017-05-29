/**
 *
 * @file
 *
 * @brief File browser view component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.browser;

import android.content.Context;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ListView;
import android.widget.TextView;

public class BrowserView extends ListView {
  
  private TextView emptyView;

  public BrowserView(Context context) {
    super(context);
    initView();
  }

  public BrowserView(Context context, AttributeSet attr) {
    super(context, attr);
    initView();
  }

  public BrowserView(Context context, AttributeSet attr, int defaultStyles) {
    super(context, attr, defaultStyles);
    initView();
  }
  
  private void initView() {
    setAdapter(new BrowserViewAdapter());
  }
  
  @Override
  public void setEmptyView(View emptyView) {
    super.setEmptyView(emptyView);
    this.emptyView = (TextView) emptyView;
  }
  
  //Use proxy structure due to view state lost on setAdapter
  final void setModel(@Nullable BrowserViewModel model) {
    ((BrowserViewAdapter) getAdapter()).setModel(model);
  }
  
  final void showError(Exception e) {
    final Throwable cause = e.getCause();
    final String msg = cause != null ? cause.getMessage() : e.getMessage();
    emptyView.setText(msg);
  }

  final void setEmptyViewText(final int res) {
    emptyView.setText(res);
  }
}
