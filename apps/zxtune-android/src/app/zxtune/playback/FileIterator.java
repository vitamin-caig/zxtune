/**
 *
 * @file
 *
 * @brief Implementation of Iterator based on Vfs objects
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import java.io.IOException;
import java.io.InvalidObjectException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;


public class FileIterator implements Iterator {
  
  private static final String TAG = FileIterator.class.getName();
  
  private static final int MAX_VISITED = 10;
  
  private final VfsIterator iterator;
  private ArrayList<VfsFile> visited;
  private int index;
  private PlayableItem item;

  public FileIterator(Context context, Uri[] paths) throws IOException {
    this.iterator = new VfsIterator(context, paths);
    this.visited = new ArrayList<VfsFile>();
    if (!next()) {
      throw new IOException(context.getString(R.string.no_tracks_found));
    }
  }
  
  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    if (index < visited.size() - 1) {
      ++index;
      return loadVisitedItem();
    } else {
      while (iterator.isValid()) {
        if (loadNextItem()) {
          return true;
        }
      }
      return false;
    }
  }
  
  @Override
  public boolean prev() {
    if (index == 0) {
      return false;
    } else {
      --index;
      return loadVisitedItem();
    }
  }
  
  private boolean loadNextItem() {
    try {
      final VfsFile file = iterator.getFile();
      iterator.next();
      item = loadItem(file);
      addVisited(file);
      return true;
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Skip not a module", e);
    } catch (IOException e) {
      Log.d(TAG, "Skip I/O error", e);
    }
    return false;
  }
  
  private boolean loadVisitedItem() {
    try {
      item = loadItem(visited.get(index));
      return true;
    } catch (IOException e) {
      Log.d(TAG, "Invalid old item", e);
      return false;
    }
  }
  
  private void addVisited(VfsFile file) {
    final int busy = visited.size();
    if (busy >= MAX_VISITED) {
      for (int idx = 1; idx != busy; ++idx) {
        visited.set(idx - 1, visited.get(idx));
      }
      visited.set(busy - 1, file);
      index = busy - 1; 
    } else {
      visited.add(file);
      index = busy;
    }
  }

  static PlayableItem loadItem(VfsFile file) throws IOException, InvalidObjectException {
    final ZXTune.Module module = loadModule(file);
    return new FileItem(file.getUri(), module);
  }
    
  static ZXTune.Module loadModule(VfsFile file) throws IOException, InvalidObjectException {
    final ByteBuffer content = file.getContent();
    return ZXTune.loadModule(content);
  }

  private static class FileItem implements PlayableItem {

    private final static String EMPTY_STRING = "";
    private ZXTune.Module module;
    private final Uri id;
    private final Uri dataId;

    public FileItem(Uri dataId, ZXTune.Module module) {
      this.module = module;
      this.id = dataId;
      this.dataId = dataId;
    }

    @Override
    public Uri getId() {
      return id;
    }

    @Override
    public Uri getDataId() {
      return dataId;
    }

    @Override
    public String getTitle() {
      return module.getProperty(ZXTune.Module.Attributes.TITLE, EMPTY_STRING);
    }

    @Override
    public String getAuthor() {
      return module.getProperty(ZXTune.Module.Attributes.AUTHOR, EMPTY_STRING);
    }
    
    @Override
    public String getProgram() {
      return module.getProperty(ZXTune.Module.Attributes.PROGRAM, EMPTY_STRING);
    }
    
    @Override
    public String getComment() {
      return module.getProperty(ZXTune.Module.Attributes.COMMENT, EMPTY_STRING);
    }

    @Override
    public TimeStamp getDuration() {
      final long frameDuration = module.getProperty(ZXTune.Properties.Sound.FRAMEDURATION, ZXTune.Properties.Sound.FRAMEDURATION_DEFAULT);
      return TimeStamp.createFrom(frameDuration * module.getDuration(), TimeUnit.MICROSECONDS);
    }

    @Override
    public ZXTune.Module getModule() {
      return module;
    }

    @Override
    public void release() {
      try {
        if (module != null) {
          module.release();
        }
      } finally {
        module = null;
      }
    }
  }
}
