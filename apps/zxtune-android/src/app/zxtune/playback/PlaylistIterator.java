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
import app.zxtune.Identifier;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.playlist.DatabaseIterator;

class PlaylistIterator implements Iterator {
  
  private static final String TAG = PlaylistIterator.class.getName();
  
  private final IteratorFactory.NavigationMode navigation;
  private DatabaseIterator delegate;
  private PlayableItem item;

  public PlaylistIterator(Context context, Uri id) throws IOException {
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
    for (DatabaseIterator it = getNext(delegate); it.isValid(); it = getNext(it)) {
      if (updateItem(it)) {
        delegate = it;
        return true;
      }
    }
    return false;
  }
  
  private DatabaseIterator getNext(DatabaseIterator it) {
    switch (navigation.get()) {
      case LOOPED:
        final DatabaseIterator next = it.getNext();
        return next.isValid() ? next : it.getFirst();
      case SHUFFLE:
        return it.getRandom();
      default:
        return it.getNext();
    }
  }

  @Override
  public boolean prev() {
    for (DatabaseIterator it = getPrev(delegate); it.isValid(); it = getPrev(it)) {
      if (updateItem(it)) {
        delegate = it;
        return true;
      }
    }
    return false;
  }

  private DatabaseIterator getPrev(DatabaseIterator it) {
    switch (navigation.get()) {
      case LOOPED:
        final DatabaseIterator prev = it.getPrev();
        return prev.isValid() ? prev : it.getLast();
      case SHUFFLE:
        return it.getRandom();
      default:
        return delegate.getPrev();
    }
  }
  
  private boolean updateItem(DatabaseIterator iter) {
    try {
      item = loadItem(iter);
      return true;
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Skip not a module", e);
    } catch (IOException e) {
      Log.d(TAG, "Skip I/O error", e);
    }
    return false;
  }
  
  private PlayableItem loadItem(DatabaseIterator iter) throws IOException, InvalidObjectException {
    final app.zxtune.playlist.Item meta = iter.getItem();
    final Identifier id = new Identifier(meta.getLocation());
    final VfsFile file = (VfsFile) Vfs.resolve(id.getDataLocation());
    if (file instanceof VfsFile) {
      final PlayableItem item = FileIterator.loadItem(file, id.getSubpath());
      return new PlaylistItem(meta, item);
    } else {
      throw new IOException("Failed to resolve " + id.getFullLocation().toString());
    }
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
