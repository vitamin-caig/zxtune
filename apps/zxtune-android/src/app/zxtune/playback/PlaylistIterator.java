/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import android.content.Context;
import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsRoot;

class PlaylistIterator extends Iterator {
  
  private final VfsRoot root;
  private app.zxtune.playlist.Iterator delegate;
  private PlayableItem item;

  public PlaylistIterator(Context context, Uri id) {
    this.root = Vfs.createRoot(context);
    this.delegate = new app.zxtune.playlist.Iterator(context, id);
    this.item = loadItem(delegate); 
  }

  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    return updateItem(delegate.getNext());
  }

  @Override
  public boolean prev() {
    return updateItem(delegate.getPrev());
  }
  
  private boolean updateItem(app.zxtune.playlist.Iterator iter) {
    if (iter.isValid()) {
      try {
        item = loadItem(iter);
        delegate = iter;
        return true;
      } catch (Error e) {
      }
    }
    return false;
  }
  
  private PlayableItem loadItem(app.zxtune.playlist.Iterator iter) {
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
