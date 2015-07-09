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

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.Identifier;
import app.zxtune.R;
import app.zxtune.Scanner;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.ZXTune.Module;
import app.zxtune.playlist.DatabaseIterator;

class PlaylistIterator implements Iterator {
  
  private static final String TAG = PlaylistIterator.class.getName();
  
  private final Scanner scanner;
  private final IteratorFactory.NavigationMode navigation;
  private DatabaseIterator delegate;
  private PlayableItem item;

  public PlaylistIterator(Context context, Uri id) throws IOException {
    this.scanner = new Scanner();
    this.navigation = new IteratorFactory.NavigationMode(context);
    this.delegate = new DatabaseIterator(context, id);
    if (!updateItem(delegate) && !next()) {
      throw new IOException(context.getString(R.string.no_tracks_found));
    }
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
  
  @Override
  public void release() {
    item.release();
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
    final PlayableItem cur = item;
    loadItem(iter);
    return cur != item;
  }
  
  private void loadItem(DatabaseIterator iter) {
    final app.zxtune.playlist.Item meta = iter.getItem();
    scanner.analyzeIdentifier(meta.getLocation(), new Scanner.Callback() {
      
      @Override
      public void onModule(Identifier id, Module module) {
        final PlayableItem fileItem = new FileIterator.FileItem(id, module);
        item = new PlaylistItem(meta, fileItem);
      }
      
      @Override
      public void onIOError(IOException e) {
        Log.d(TAG, "Ignore I/O error", e);
      }
    });
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
    public Identifier getDataId() {
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
