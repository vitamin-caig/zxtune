/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;

public class FileIterator extends Iterator {
  
  private final VfsIterator iterator;
  private PlayableItem item;

  public FileIterator(Context context, Uri path) {
    this.iterator = new VfsIterator(context, path);
    try {
      this.item = loadItem(iterator.getFile());
    } catch (Error e) {
      if (!next()) {
        throw e;
      }
    }
  }
  
  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    return advance(1);
  }
  
  @Override
  public boolean prev() {
    return advance(-1);
  }
  
  private boolean advance(int delta) {
    //skip invalid items at all
    final int initialPos = iterator.getPos();
    for (int curPos = initialPos + delta; ; curPos += delta) {
      iterator.setPos(curPos);
      if (!iterator.isValid()) {
        break;
      }
      if (loadNewItem()) {
        return true;
      }
    }
    iterator.setPos(initialPos);
    return false;
  }

  private boolean loadNewItem() {
    try {
      item = loadItem(iterator.getFile());
      return true;
    } catch (Error e) {
    }
    return false;
  }

  static PlayableItem loadItem(VfsFile file) {
    final ZXTune.Module module = loadModule(file);
    return new FileItem(file.getUri(), module);
  }
    
  static ZXTune.Module loadModule(VfsFile file) {
    try {
      final byte[] content = file.getContent();
      return ZXTune.loadModule(content);
    } catch (IOException e) {
      throw new Error(e.getCause());
    } catch (RuntimeException e) {
      throw new Error(e.getCause());
    }
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
