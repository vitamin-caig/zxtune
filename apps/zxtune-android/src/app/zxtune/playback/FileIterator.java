/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;

public class FileIterator extends Iterator {
  
  private final PlayableItem item;

  public FileIterator(Uri path) throws IOException {
    this.item = loadItem(path);
  }
  
  @Override
  public PlayableItem getItem() {
    return item;
  }

  @Override
  public boolean next() {
    return false;
  }

  @Override
  public boolean prev() {
    return false;
  }

  static PlayableItem loadItem(Uri path) throws IOException {
    final ZXTune.Module module = loadModule(path);
    return new FileItem(path, module);
  }
    
  public static ZXTune.Module loadModule(Uri path) throws IOException {
    try {
      final byte[] content = loadFile(path.getPath());
      return ZXTune.loadModule(content);
    } catch (RuntimeException e) {
      throw new IOException(e.toString());
    }
  }

  private static byte[] loadFile(String path) throws IOException {
    final File file = new File(path);
    final FileInputStream stream = new FileInputStream(file);
    try {
      final int size = (int) file.length();
      byte[] result = new byte[size];
      stream.read(result, 0, size);
      return result;
    } finally {
      stream.close();
    }
  }
  
  private static class FileItem implements PlayableItem {

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
      return module.getProperty(ZXTune.Module.Attributes.TITLE, "");
    }

    @Override
    public String getAuthor() {
      return module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
    }

    @Override
    public TimeStamp getDuration() {
      return TimeStamp.createFrom(20 * module.getDuration(), TimeUnit.MILLISECONDS);
    }

    @Override
    public ZXTune.Player createPlayer() {
      return module.createPlayer();
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
