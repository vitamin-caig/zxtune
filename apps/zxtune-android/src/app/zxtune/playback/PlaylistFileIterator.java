/**
 *
 * @file
 *
 * @brief Implementation of Iterator based on playlist file
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;

import android.content.Context;
import android.net.Uri;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playback.IteratorFactory.NavigationMode;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.XspfIterator;

final class PlaylistFileIterator implements Iterator {

  private final VfsRoot root;
  //use java.net uri for correct resoling of relative paths
  private final URI dir;
  private final ReferencesIterator delegate;
  private final IteratorFactory.NavigationMode navigation;
  private PlayableItem item;
  
  enum Type {
    UNKNOWN,
    XSPF,
    AYL
  }
  
  public static Iterator create(Context context, Uri path) throws IOException {
    final Type type = detectType(path.getLastPathSegment());
    if (type == Type.UNKNOWN) {
      return null;
    }
    final VfsRoot root = Vfs.createRoot(context);
    final VfsFile file = (VfsFile) root.resolve(path);
    final ReferencesIterator delegate = createDelegate(type, file.getContent());
    final IteratorFactory.NavigationMode navigation = new IteratorFactory.NavigationMode(context);
    return new PlaylistFileIterator(root, getParentOf(path), delegate, navigation);
  }
  
  private static Type detectType(String filename) {
    if (filename.endsWith(".xspf")) {
      return Type.XSPF;
    } else if (filename.endsWith(".ayl")) {
      return Type.AYL;
    } else {
      return Type.UNKNOWN;
    }
  }
  
  private static ReferencesIterator createDelegate(Type type, ByteBuffer buf) throws IOException {
    switch (type) {
      case XSPF:
        return XspfIterator.create(buf);
      case AYL:
        return AylIterator.create(buf);
      default:
        return null;
    }
  }
  
  private PlaylistFileIterator(VfsRoot root, URI dir, ReferencesIterator delegate, IteratorFactory.NavigationMode navigation) throws IOException {
    this.root = root;
    this.dir = dir;
    this.delegate = delegate;
    this.navigation = navigation;
    if (!next()) {
      throw new IOException("No items to play");
    }
  }
  
  private static URI getParentOf(Uri uri) {
    return URI.create(uri.toString()).resolve(".");
  }

  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    while (getNext()) {
      if (loadNewItem()) {
        return true;
      }
    }
    return false;
  }
  
  private boolean getNext() {
    switch (navigation.get()) {
      case LOOPED:
        return delegate.next() || delegate.first();
      case SHUFFLE:
        return delegate.random();
      default:
        return delegate.next();
    }
  }

  @Override
  public boolean prev() {
    while (getPrev()) {
      if (loadNewItem()) {
        return true;
      }
    }
    return false;
  }
  
  private boolean getPrev() {
    switch (navigation.get()) {
      case LOOPED:
        return delegate.prev() || delegate.last();
      case SHUFFLE:
        return delegate.random();
      default:
        return delegate.prev();
    }
  }
  
  private boolean loadNewItem() {
    try {
      final String location = delegate.getItem().location;
      Uri uri = Uri.parse(Uri.encode(location, "/"));
      if (isWindowsPath(location)) {
        return false;//windows paths are not supported
      } else if (!isAbsolutePath(location)) {
        uri = Uri.parse(dir.resolve(uri.toString()).toString());
      }
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
  
  private boolean isWindowsPath(String path) {
    return path.length() > 2 && path.charAt(1) == ':';
  }
  
  private boolean isAbsolutePath(String path) {
    return !path.isEmpty() && path.startsWith(File.separator);
  }
}
