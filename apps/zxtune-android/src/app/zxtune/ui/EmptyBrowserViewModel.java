/**
 * 
 * @file
 *
 * @brief Empty implementation of BrowserViewModel
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui;

import android.view.View;
import android.view.ViewGroup;

class EmptyBrowserViewModel implements BrowserViewModel {

  @Override
  public int getCount() {
    return 0;
  }

  @Override
  public boolean isEmpty() {
    return true;
  }

  @Override
  public Object getItem(int position) {
    return null;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    return convertView;
  }
}
