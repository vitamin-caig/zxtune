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
import java.util.ArrayDeque;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.Identifier;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.ZXTune.Module;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;


public class FileIterator implements Iterator {
  
  private static final String TAG = FileIterator.class.getName();
  
  private static class CacheEntry {

    final VfsFile file;
    final String subpath;
    PlayableItem item;
    
    CacheEntry(VfsFile file, String subpath, PlayableItem item) {
      this.file = file;
      this.subpath = subpath;
      this.item = item;
    }
    
    final PlayableItem captureItem() {
      final PlayableItem result = item;
      item = null;
      return result;
    }
  }
  
  private static final int MAX_VISITED = 10;
  
  private final VfsIterator iterator;
  private final ArrayDeque<CacheEntry> prev;
  private final ArrayDeque<CacheEntry> next;//first is current

  public FileIterator(Context context, Uri[] paths) throws IOException {
    this.iterator = new VfsIterator(context, paths);
    this.prev = new ArrayDeque<CacheEntry>();
    this.next = new ArrayDeque<CacheEntry>();
    prefetch();
    if (next.isEmpty()) {
      throw new IOException(context.getString(R.string.no_tracks_found));
    }
  }
  
  @Override
  public PlayableItem getItem() {
    final CacheEntry entry = next.getFirst();
    final PlayableItem cached = entry.captureItem();
    if (cached != null) {
      return cached;
    }
    return loadItem(entry);
  }

  @Override
  public boolean next() {
    if (hasNext()) {
      final CacheEntry entry = next.removeFirst();
      prev.addFirst(entry);
      assert null == entry.captureItem();
      cleanupHistory();
      prefetch();
      return true;
    } else {
      return false;
    }
  }
  
  private boolean hasNext() {
    return next.size() >= 2;
  }
  
  @Override
  public boolean prev() {
    if (prev.isEmpty()) {
      return false;
    } else {
      final CacheEntry entry = prev.removeFirst();
      next.addFirst(entry);
      return true;
    }
  }
  
  private void prefetch() {
    while (!hasNext() && iterator.isValid()) {
      processNextFile();
    }
  }
  
  private PlayableItem loadItem(CacheEntry entry) {
    try {
      return loadItem(entry.file, entry.subpath);
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Cached item become invalid");
    } catch (IOException e) {
      Log.d(TAG, "I/O error while loading cached item");
    }
    return PlayableItemStub.instance();
  }
  
  private void processNextFile() {
    try {
      final VfsFile file = iterator.getFile();
      iterator.next();
      detectItems(file);
    } catch (InvalidObjectException e) {
      Log.d(TAG, "Skip not a module", e);
    } catch (IOException e) {
      Log.d(TAG, "Skip I/O error", e);
    }
  }
  
  private void detectItems(final VfsFile file) throws IOException {
    final ByteBuffer content = file.getContent();
    final Uri uri = file.getUri();
    ZXTune.detectModules(content, new ZXTune.ModuleDetectCallback() {
      
      @Override
      public void onModule(String subpath, Module obj) {
        final PlayableItem item = new FileItem(uri, subpath, obj);
        final CacheEntry entry = new CacheEntry(file, subpath, item);
        next.addLast(entry);
      }
    });
  }
  
  private void cleanupHistory() {
    while (prev.size() > MAX_VISITED) {
      prev.removeLast();
    }
  }
  
  static PlayableItem loadItem(VfsFile file, String subpath) throws IOException, InvalidObjectException {
    final ZXTune.Module module = loadModule(file, subpath);
    return new FileItem(file.getUri(), subpath, module);
  }
    
  static ZXTune.Module loadModule(VfsFile file, String subpath) throws IOException, InvalidObjectException {
    final ByteBuffer content = file.getContent();
    return ZXTune.loadModule(content, subpath);
  }

  private static class FileItem implements PlayableItem {

    private final static String EMPTY_STRING = "";
    private ZXTune.Module module;
    private final Uri id;
    private final Uri dataId;

    public FileItem(Uri fileId, String subpath, ZXTune.Module module) {
      this.module = module;
      this.id = new Identifier(fileId, subpath).getFullLocation();
      this.dataId = this.id;
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
