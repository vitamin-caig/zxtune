/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import java.io.IOException;

import android.content.Context;
import android.net.Uri;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playlist.XspfIterator;

/**
 * 
 */
final class XspfPlaylistIterator extends Iterator {

  private final VfsRoot root;
  private final XspfIterator delegate;
  private final Uri dir;
  private PlayableItem item;
  
  public XspfPlaylistIterator(Context context, Uri path) throws IOException {
    this.root = Vfs.createRoot(context);
    final VfsFile file = (VfsFile) root.resolve(path);
    this.delegate = new XspfIterator(file.getContent());
    this.dir = getParentOf(path);
    if (!next()) {
      throw new IOException("No items to play");
    }
  }
  
  private static Uri getParentOf(Uri uri) {
    final String filename = uri.getLastPathSegment();
    final String str = uri.toString();
    final int filenamePos = str.lastIndexOf(filename);
    return Uri.parse(str.substring(0, filenamePos));
  }

  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    while (delegate.next()) {
      if (loadNewItem()) {
        return true;
      }
    }
    return false;
  }

  @Override
  public boolean prev() {
    while (delegate.prev()) {
      if (loadNewItem()) {
        return true;
      }
    }
    return false;
  }
  
  private boolean loadNewItem() {
    try {
      final String location = delegate.getItem().location;
      final Uri uri = location.startsWith("/")
        ? Uri.parse(location)
        : Uri.withAppendedPath(dir, location);
      final VfsObject obj = root.resolve(uri);
      if (obj instanceof VfsFile) {
        item = FileIterator.loadItem((VfsFile) obj);
        return true;
      }
    } catch (IOException e) {
    } catch (Error e) {
      //from loadItem
    }
    return false;
  }
}
