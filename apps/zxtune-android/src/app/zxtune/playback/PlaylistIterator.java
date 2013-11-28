/**
 *
 * @file
 *
 * @brief Implementation of Iterator based on playlist database
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import java.io.IOException;
import java.io.InvalidObjectException;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsRoot;
import app.zxtune.playlist.DatabaseIterator;

class PlaylistIterator implements Iterator {
  
  private static final String TAG = PlaylistIterator.class.getName();
  
  private final VfsRoot root;
  private final IteratorFactory.NavigationMode navigation;
  private DatabaseIterator delegate;
  private PlayableItem item;

  public PlaylistIterator(Context context, Uri id) throws IOException {
    this.root = Vfs.createRoot(context);
    this.navigation = new IteratorFactory.NavigationMode(context);
    this.delegate = new DatabaseIterator(context, id);
    this.item = loadItem(delegate); 
  }

  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    return updateItem(getNext());
  }
  
  private DatabaseIterator getNext() {
    switch (navigation.get()) {
      case LOOPED:
        final DatabaseIterator next = delegate.getNext();
        return next.isValid() ? next : delegate.getFirst();
      case SHUFFLE:
        return delegate.getRandom();
      default:
        return delegate.getNext();
    }
  }

  @Override
  public boolean prev() {
    return updateItem(getPrev());
  }

  private DatabaseIterator getPrev() {
    switch (navigation.get()) {
      case LOOPED:
        final DatabaseIterator prev = delegate.getPrev();
        return prev.isValid() ? prev : delegate.getLast();
      case SHUFFLE:
        return delegate.getRandom();
      default:
        return delegate.getPrev();
    }
  }
  
  private boolean updateItem(DatabaseIterator iter) {
    if (iter.isValid()) {
      try {
        item = loadItem(iter);
        delegate = iter;
        return true;
      } catch (InvalidObjectException e) {
        Log.d(TAG, "Skip not a module", e);
      } catch (IOException e) {
        Log.d(TAG, "Skip I/O error", e);
      }
    }
    return false;
  }
  
  private PlayableItem loadItem(DatabaseIterator iter) throws IOException, InvalidObjectException {
    final app.zxtune.playlist.Item meta = iter.getItem();
    final VfsFile file = (VfsFile) root.resolve(meta.getLocation());
    final PlayableItem item = FileIterator.loadItem(file);
    return new PlaylistItem(meta, item);
  }
  
  private static class PlaylistItem implements PlayableItem {
    
    private final app.zxtune.playlist.Item meta;
    private final PlayableItem content;
    
    public PlaylistItem(app.zxtune.playlist.Item meta, PlayableItem content) {
      this.meta = meta;
      this.content = content;
    }

    @Override
    public Uri getId() {
      return meta.getUri();
    }

    @Override
    public Uri getDataId() {
      return content.getDataId();
    }

    @Override
    public String getTitle() {
      return meta.getTitle();
    }

    @Override
    public String getAuthor() {
      return meta.getAuthor();
    }
    
    @Override
    public String getProgram() {
      return content.getProgram();
    }

    @Override
    public String getComment() {
      return content.getComment();
    }
    
    @Override
    public TimeStamp getDuration() {
      return meta.getDuration();
    }
    
    @Override
    public ZXTune.Module getModule() {
      return content.getModule();
    }
    
    @Override
    public void release() {
      content.release();
    }
  }
}
