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

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.ui.utils.ListItemViewHolder;

class BrowserViewHolder {

  private final ListItemViewHolder delegate;

  private BrowserViewHolder(ListItemViewHolder delegate) {
    this.delegate = delegate;
  }

  static View createView(Context context, ViewGroup parent) {
    return ListItemViewHolder.createView(context, parent);
  }

  static BrowserViewHolder fromView(View view) {
    return new BrowserViewHolder(ListItemViewHolder.fromView(view));
  }

  final void fill(VfsObject obj) {
    setNameDescription(obj);
    if (obj instanceof VfsDir) {
      setIcon(obj);
    } else {
      setSize((VfsFile) obj);
    }
  }

  private void setNameDescription(VfsObject obj) {
    delegate.setMainText(obj.getName());
    delegate.setAuxText(obj.getDescription());
  }

  private void setIcon(VfsObject dir) {
    final int iconId = getIcon(dir);
    delegate.setIcon(iconId);
    delegate.setDetailText(null);
  }

  private void setSize(VfsFile file) {
    delegate.setIcon(null);
    delegate.setDetailText(file.getSize());
  }

  private static int getIcon(VfsObject dir) {
    final Object icon = dir.getExtension(VfsExtensions.ICON_RESOURCE);
    return icon != null
      ? (Integer) icon
      : R.drawable.ic_browser_folder;
  }
}
