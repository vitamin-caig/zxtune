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
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.Identifier;
import app.zxtune.R;
import app.zxtune.Scanner;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.ZXTune.Module;

public class FileIterator implements Iterator {
  
  private static final String TAG = FileIterator.class.getName();
  
  private static final int MAX_VISITED = 10;
  
  private final Scanner scanner;
  private final ExecutorService executor;
  private final LinkedBlockingQueue<PlayableItem> itemsQueue;
  private final ArrayList<Identifier> history;
  private int historyDepth;
  private IOException lastError;
  private PlayableItem item;
  
  public FileIterator(Context context, Uri[] uris) throws IOException {
    this.scanner = new Scanner();
    this.executor = Executors.newCachedThreadPool();
    this.itemsQueue = new LinkedBlockingQueue<PlayableItem>(1);
    this.history = new ArrayList<Identifier>();
    this.historyDepth = 0;
    start(uris);
    if (!takeNextItem()) {
      if (lastError != null) {
        throw lastError;
      }
      throw new IOException(context.getString(R.string.no_tracks_found));
    }
  }
  
  @Override
  public PlayableItem getItem() {
    return item;
  }
  
  @Override
  public boolean next() {
    while (0 != historyDepth) {
      if (loadFrom(history.get(--historyDepth))) {
        return true;
      }
    }
    if (takeNextItem()) {
      while (history.size() > MAX_VISITED) {
        history.remove(MAX_VISITED);
      }
      return true;
    }
    return false;
  }
  
  @Override
  public boolean prev() {
    while (historyDepth + 1 < history.size()) {
      if (loadFrom(history.get(++historyDepth))) {
        return true;
      }
    }
    return false;
  }
  
  @Override
  public void release() {
    stop();
    for (;;) {
      final PlayableItem item = itemsQueue.poll();
      if (item != null) {
        item.release();
      } else {
        break;
      }
    }
  }
  
  private void start(final Uri[] uris) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        try {
          scanner.analyze(uris, new Scanner.Callback() {
            @Override
            public void onModule(Identifier id, Module module) {
              addItem(new FileItem(id, module));
            }
            
            @Override
            public void onIOError(IOException e) {
              lastError = e;
            }
          });
          addItem(PlayableItemStub.instance());//limiter
        } catch (Error e) {
        }
      }
    });
  }
  
  private void stop() {
    try {
      do {
        executor.shutdownNow();
        Log.d(TAG, "Waiting for executor shutdown...");
      } while (!executor.awaitTermination(10, TimeUnit.SECONDS));
      Log.d(TAG, "Executor shut down");
    } catch (InterruptedException e) {
      Log.w(TAG, "Failed to shutdown executor", e);
    }
  }
  
  private void addItem(PlayableItem item) {
    try {
      itemsQueue.put(item);
    } catch (InterruptedException e) {
      throw new Error("Interrupted");
    }
  }
  
  private boolean takeNextItem() {
    try {
      item = itemsQueue.take();
      if (item != PlayableItemStub.instance()) {
        history.add(0, item.getDataId());
        return true;
      }
      //put limiter back
      itemsQueue.put(item);
    } catch (InterruptedException e) {
    }
    return false;
  }
  
  private boolean loadFrom(Identifier id) {
    final PlayableItem current = item;
    scanner.analyzeIdentifier(id, new Scanner.Callback() {
      
      @Override
      public void onModule(Identifier id, Module module) {
        item = new FileItem(id, module);
      }
      
      @Override
      public void onIOError(IOException e) {
      }
    });
    return item != current;
  }
  
  static class FileItem implements PlayableItem {

    private final static String EMPTY_STRING = "";
    private ZXTune.Module module;
    private final Uri id;
    private final Identifier dataId;

    public FileItem(Identifier id, ZXTune.Module module) {
      this.module = module;
      this.id = id.getFullLocation();
      this.dataId = id;
    }

    @Override
    public Uri getId() {
      return id;
    }

    @Override
    public Identifier getDataId() {
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
