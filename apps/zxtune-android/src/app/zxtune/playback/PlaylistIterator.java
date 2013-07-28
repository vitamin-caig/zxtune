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
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;

class PlaylistIterator extends Iterator {
  
  private final app.zxtune.playlist.Iterator delegate;
  private PlayableItem item;

  public PlaylistIterator(Context context, Uri id) {
    this.delegate = new app.zxtune.playlist.Iterator(context, id);
    this.item = loadItem(); 
  }

  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    delegate.next();
    return updateItem();
  }

  @Override
  public boolean prev() {
    delegate.prev();
    return updateItem();
  }
  
  private boolean updateItem() {
    if (delegate.isValid()) {
      final PlayableItem newItem = loadItem();
      if (newItem != null) {
        item = newItem;
        return true;
      }
    }
    return false;
  }

  private PlayableItem loadItem() {
    try {
      final app.zxtune.playlist.Item meta = delegate.getCurrentItem();
      final PlayableItem file = FileIterator.loadItem(meta.getLocation());
      return new PlaylistItem(meta, file);
    } catch (IOException e) {
      return null;
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
    public ZXTune.Player createPlayer() {
      return content.createPlayer();
    }
    
    @Override
    public void release() {
      content.release();
    }
  }
}
