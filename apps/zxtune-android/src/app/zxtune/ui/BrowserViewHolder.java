/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

class BrowserViewHolder {

  private final ImageView icon;
  private final TextView name;
  private final TextView description;
  private final TextView size;
  
  BrowserViewHolder(View view) {
    this.icon = (ImageView) view.findViewById(R.id.browser_item_icon);
    this.name = (TextView) view.findViewById(R.id.browser_item_name);
    this.description = (TextView) view.findViewById(R.id.browser_item_description);
    this.size = (TextView) view.findViewById(R.id.browser_item_size);
  }
  
  final void makeView(VfsDir dir) {
    makeBaseView(dir);
    final int iconId = getIcon(dir);
    if (icon != null) {
      icon.setVisibility(View.VISIBLE);
      icon.setImageResource(iconId);
    } else {
      name.setCompoundDrawablesWithIntrinsicBounds(iconId, 0, 0, 0);
    }
    size.setVisibility(View.GONE);
  }
  
  final void makeView(VfsFile file) {
    makeBaseView(file);
    if (icon != null) {
      icon.setVisibility(View.GONE);
    } else {
      name.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
    }
    size.setVisibility(View.VISIBLE);
    size.setText(file.getSize());
  }
  
  private void makeBaseView(VfsObject obj) {
    name.setText(obj.getName());
    if (description != null) {
      description.setText(obj.getDescription());
    }
  }
  
  private static int getIcon(VfsDir dir) {
    return dir instanceof IconSource
      ? ((IconSource) dir).getResourceId()
      : R.drawable.ic_browser_folder;
  }
}
