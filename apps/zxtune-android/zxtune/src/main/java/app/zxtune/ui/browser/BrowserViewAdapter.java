/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

class BrowserViewAdapter extends BaseAdapter {

  private BrowserViewModel model;
  
  BrowserViewAdapter() {
    this.model = getSafeModel(null);
  }

  @Override
  public int getCount() {
    return model.getCount();
  } 
    
  @Override
  @Nullable
  public Object getItem(int position) {
    return model.getItem(position);
  }
  
  @Override
  public long getItemId(int position) {
    return position;
  }
  
  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    return model.getView(position, convertView, parent);
  }

  @Override
  public boolean isEmpty() {
    return model.isEmpty();
  }

  final void setModel(@Nullable BrowserViewModel model) {
    setInitialModel(model);
    if (model != null) {
      notifyDataSetChanged();
    } else {
      notifyDataSetInvalidated();
    }
  }
    
  private void setInitialModel(@Nullable BrowserViewModel model) {
    if (this.model != model) {
      this.model = getSafeModel(model);
    }
  }
  
  private static BrowserViewModel getSafeModel(@Nullable BrowserViewModel model) {
    if (model != null) {
      return model;
    } else {
      return new EmptyBrowserViewModel();
    }
  }
}
