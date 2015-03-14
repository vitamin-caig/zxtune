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
import app.zxtune.Identifier;
import app.zxtune.R;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator;
import app.zxtune.playlist.XspfIterator;

//TODO: cleanup error processing logic
final class PlaylistFileIterator implements Iterator {
  
  private final static String TAG = PlaylistFileIterator.class.getName();

  private final VfsRoot root;
  //use java.net.URI for correct resolving of relative paths
  private final URI dir;
  private final ReferencesIterator delegate;
  private final VfsIterator.ErrorHandler handler;
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
    final VfsRoot root = Vfs.getRoot();
    final VfsFile file = (VfsFile) root.resolve(path);
    final ReferencesIterator delegate = createDelegate(type, file.getContent());
    final VfsIterator.KeepLastErrorHandler handler = new VfsIterator.KeepLastErrorHandler();
    final PlaylistFileIterator result = new PlaylistFileIterator(root, getParentOf(path), delegate, handler);
    if (!result.next()) {
      handler.throwLastIOError();
      throw new IOException(context.getString(R.string.no_tracks_found));
    }
    return result;
  }
  
  private static Type detectType(String filename) {
    if (filename == null) {
      return Type.UNKNOWN;
    } if (filename.endsWith(".xspf")) {
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
  
  private PlaylistFileIterator(VfsRoot root, URI dir, ReferencesIterator delegate, VfsIterator.ErrorHandler handler) {
    this.root = root;
    this.dir = dir;
    this.delegate = delegate;
    this.handler = handler;
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
      Log.d(TAG, location + " => " + uri);
      final Identifier id = new Identifier(uri);
      final VfsObject obj = root.resolve(id.getDataLocation());
      if (obj instanceof VfsFile) {
        item = FileIterator.loadItem((VfsFile) obj, id.getSubpath());
        return true;
      }
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Skip not a module", e);
    } catch (IOException e) {
      handler.onIOError(e);
    }
    return false;
  }
  
  private boolean isWindowsPath(String path) {
    return path.length() > 2 && path.charAt(1) == ':';
  }
}
