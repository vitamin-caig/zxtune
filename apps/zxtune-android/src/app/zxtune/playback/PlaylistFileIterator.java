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

import java.io.IOException;
import java.io.InvalidObjectException;
import java.net.URI;
import java.nio.ByteBuffer;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.XspfIterator;

final class PlaylistFileIterator implements Iterator {
  
  private final static String TAG = PlaylistFileIterator.class.getName();

  private final VfsRoot root;
  //use java.net.URI for correct resolving of relative paths
  private final URI dir;
  private final ReferencesIterator delegate;
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
    final PlaylistFileIterator result = new PlaylistFileIterator(root, getParentOf(path), delegate);
    if (result.initialize()) {
      return result;
    }
    throw new IOException(context.getString(R.string.no_tracks_found));
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
  
  private PlaylistFileIterator(VfsRoot root, URI dir, ReferencesIterator delegate) {
    this.root = root;
    this.dir = dir;
    this.delegate = delegate;
  }
  
  final boolean initialize() {
    while (delegate.next()) {
      if (loadNewItem()) {
        return true;
      }
    }
    return false;
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
      if (isWindowsPath(location)) {
        return false;//windows paths are not supported
      }
      final Uri uri = Uri.parse(dir.resolve(location).toString());
      final VfsObject obj = root.resolve(uri);
      if (obj instanceof VfsFile) {
        item = FileIterator.loadItem((VfsFile) obj);
        return true;
      }
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Skip not a module", e);
    } catch (IOException e) {
      Log.d(TAG, "Skip I/O error", e);
    }
    return false;
  }
  
  private boolean isWindowsPath(String path) {
    return path.length() > 2 && path.charAt(1) == ':';
  }
}
