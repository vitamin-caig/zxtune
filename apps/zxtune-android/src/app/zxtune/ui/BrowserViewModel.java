/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

abstract class BrowserViewModel {
  
  abstract int getCount();
  
  abstract boolean isEmpty();
  
  abstract Object getItem(int position);
  
  abstract View getView(int position, View convertView, ViewGroup parent);
}

class EmptyBrowserViewModel extends BrowserViewModel {

  @Override
  int getCount() {
    return 0;
  }

  @Override
  boolean isEmpty() {
    return true;
  }

  @Override
  Object getItem(int position) {
    return null;
  }

  @Override
  View getView(int position, View convertView, ViewGroup parent) {
    return convertView;
  }
}

//TODO: synchronize?
class RealBrowserViewModel extends BrowserViewModel {
  
  private final static Comparator<VfsObject> COMPARE_BY_NAME = new Comparator<VfsObject>() {

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      return String.CASE_INSENSITIVE_ORDER.compare(lh.getName(), rh.getName());
    }
  };
  
  private final LayoutInflater inflater;
  private final ArrayList<VfsDir> dirs;
  private final ArrayList<VfsFile> files;
  
  RealBrowserViewModel(Context context) {
    this.inflater = LayoutInflater.from(context);
    this.dirs = new ArrayList<VfsDir>();
    this.files = new ArrayList<VfsFile>();
  }
  
  final void add(VfsDir dir) {
    dirs.add(dir);
  }
  
  final void add(VfsFile file) {
    files.add(file);
  }
  
  final void sort() {
    Collections.sort(dirs, COMPARE_BY_NAME);
    Collections.sort(files, COMPARE_BY_NAME);
  }
  
  @Override
  int getCount() {
    return dirs.size() + files.size();
  }

  @Override
  boolean isEmpty() {
    return dirs.isEmpty() && files.isEmpty();
  }
    
  @Override
  Object getItem(int position) {
    final int dirsCount = dirs.size();
    if (position < dirsCount) {
      return dirs.get(position);
    } else {
      return files.get(position - dirsCount);
    }
  }

  @Override
  //TODO: use explicit multiple view types
  View getView(int position, View convertView, ViewGroup parent) {
    View result;
    ViewHolder holder = null;
    if (convertView == null) {
      result = inflater.inflate(R.layout.dirview_item, parent, false);
      holder = new ViewHolder(result);
      result.setTag(holder);
    } else {
      result = convertView;
      holder = (ViewHolder) convertView.getTag();
    }

    final int dirsCount = dirs.size();
    if (position < dirsCount) {
      holder.makeView(dirs.get(position));
    } else {
      holder.makeView(files.get(position - dirsCount));
    }

    return result;
  }

  private static class ViewHolder {

    private final TextView name;
    private final TextView size;
    
    ViewHolder(View view) {
      this.name = (TextView) view.findViewById(R.id.dirview_item_name);
      this.size = (TextView) view.findViewById(R.id.dirview_item_size);
    }
    
    void makeView(VfsDir dir) {
      makeBaseView(dir);
      final int icon = getIcon(dir);
      name.setCompoundDrawablesWithIntrinsicBounds(icon, 0, 0, 0);
      size.setVisibility(View.GONE);
    }
    
    void makeView(VfsFile file) {
      makeBaseView(file);
      name.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
      size.setVisibility(View.VISIBLE);
      size.setText(file.getSize());
    }
    
    private void makeBaseView(VfsObject obj) {
      name.setText(obj.getName());
    }
    
    private static int getIcon(VfsDir dir) {
      final VfsDir.Icon ic = dir.getClass().getAnnotation(VfsDir.Icon.class);
      return ic != null
        ? ic.value()
        : R.drawable.ic_browser_folder;
    }
  }
}
