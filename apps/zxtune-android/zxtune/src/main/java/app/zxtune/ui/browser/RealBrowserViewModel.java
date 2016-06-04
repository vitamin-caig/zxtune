/**
 *
 * @file
 *
 * @brief Real sorted file browser model
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.browser;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

//TODO: synchronize?
class RealBrowserViewModel implements BrowserViewModel {
  
  private final Context context;
  private final ArrayList<VfsDir> dirs;
  private final ArrayList<VfsFile> files;
  
  RealBrowserViewModel(Context context) {
    this.context = context;
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
    sort(DefaultComparator.instance());
  }
  
  final void sort(Comparator<VfsObject> comparator) {
    Collections.sort(dirs, comparator);
    Collections.sort(files, comparator);
  }
  
  @Override
  public int getCount() {
    return dirs.size() + files.size();
  }

  @Override
  public boolean isEmpty() {
    return dirs.isEmpty() && files.isEmpty();
  }
    
  @Override
  public Object getItem(int position) {
    final int dirsCount = dirs.size();
    if (position < dirsCount) {
      return dirs.get(position);
    } else {
      return files.get(position - dirsCount);
    }
  }

  @Override
  //TODO: use explicit multiple view types
  public View getView(int position, View convertView, ViewGroup parent) {
    final View view = convertView == null
            ? BrowserViewHolder.createView(context, parent)
            : convertView;
    final BrowserViewHolder holder = BrowserViewHolder.fromView(view);
    final int dirsCount = dirs.size();
    if (position < dirsCount) {
      holder.fill(dirs.get(position));
    } else {
      holder.fill(files.get(position - dirsCount));
    }
   return view;
  }
}