/**
 * 
 * @file
 *
 * @brief BrowserView items holder
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

import android.annotation.SuppressLint;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.ui.IconSource;

@SuppressLint("Assert")
class BrowserViewHolder {

  private final ImageView icon;
  private final TextView name;
  private final TextView description;
  private final TextView size;
  
  private BrowserViewHolder(View view) {
    this.icon = (ImageView) view.findViewById(R.id.browser_item_icon);
    this.name = (TextView) view.findViewById(R.id.browser_item_name);
    this.description = (TextView) view.findViewById(R.id.browser_item_description);
    this.size = (TextView) view.findViewById(R.id.browser_item_size);
  }
  
  static BrowserViewHolder create(View view) {
    return new BrowserViewHolder(view);
  }
  
  final void fill(VfsObject obj) {
    setNameDescription(obj);
    if (obj instanceof VfsDir) {
      setIcon(obj);
      size.setVisibility(View.GONE);
    } else {
      icon.setVisibility(View.GONE);
      setSize((VfsFile) obj);
    }
  }

  private void setNameDescription(VfsObject obj) {
    name.setText(obj.getName());
    description.setText(obj.getDescription());
  }

  private void setIcon(VfsObject dir) {
    final int iconId = getIcon(dir);
    icon.setImageResource(iconId);
    icon.setVisibility(View.VISIBLE);
  }

  private void setSize(VfsFile file) {
    size.setText(file.getSize());
    size.setVisibility(View.VISIBLE);
  }

  private static int getIcon(VfsObject dir) {
    return dir instanceof IconSource
      ? ((IconSource) dir).getResourceId()
      : R.drawable.ic_browser_folder;
  }
}
