/**
 *
 * @file
 *
 * @brief File browser model interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.browser;

import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;

interface BrowserViewModel {
  
  int getCount();
  
  boolean isEmpty();

  @Nullable
  Object getItem(int position);
  
  View getView(int position, View convertView, ViewGroup parent);
}
