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

import android.content.Context;
import android.net.Uri;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.Scanner;
import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Properties;
import app.zxtune.playback.stubs.PlayableItemStub;

public class FileIterator implements Iterator {
  
  private static final String TAG = FileIterator.class.getName();
  
  private static final int MAX_VISITED = 10;
  
  private final Scanner scanner;
  private final ExecutorService executor;
  private final LinkedBlockingQueue<PlayableItem> itemsQueue;
  private final ArrayList<Identifier> history;
  private int historyDepth;
  private Exception lastError;
  private PlayableItem item;
  
  public FileIterator(Context context, Uri[] uris) throws Exception {
    this.scanner = new Scanner();
    this.executor = Executors.newCachedThreadPool();
    this.itemsQueue = new LinkedBlockingQueue<>(1);
    this.history = new ArrayList<>();
    this.historyDepth = 0;
    final boolean hasRestFiles = uris.length > 1;
    if (hasRestFiles) {
      start(Arrays.copyOfRange(uris, 0, 1), false);
    } else {
      start(uris, true);
    }
    if (!takeNextItem()) {
      if (lastError != null) {
        throw lastError;
      }
      throw new Exception(context.getString(R.string.no_tracks_found));
    }
    if (hasRestFiles) {
      start(Arrays.copyOfRange(uris, 1, uris.length), true);
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
  
  private void start(final Uri[] uris, final boolean additionalLimiter) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        try {
          final int[] modules = {0};
          scanner.analyze(uris, new Scanner.Callback() {
            @Override
            public void onModule(Identifier id, Module module) {
              addItem(new FileItem(id, module));
              ++modules[0];
            }

            @Override
            public void onError(Exception e) {
              lastError = e;
            }
          });
          if (modules[0] == 0 || additionalLimiter) {
            addItem(PlayableItemStub.instance());
          }
        } catch (Error e) {//use unchecked exception to process interruptions
          if (e.getCause() instanceof InterruptedException) {
            Log.d(TAG, "Interrupted");
          } else {
            Log.w(TAG, e, "Error in FileIterator.start()");
          }
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
      Log.w(TAG, e, "Failed to shutdown executor");
    }
  }
  
  private void addItem(PlayableItem item) {
    try {
      itemsQueue.put(item);
    } catch (InterruptedException e) {
      throw new Error("Interrupted", e);
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
      Log.d(TAG, "Interrupted FileIterator.takeNextItem");
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
      public void onError(Exception e) {
        Log.w(TAG, e, "Error");
      }
    });
    return item != current;
  }
  
  static class FileItem implements PlayableItem {

    private static final String EMPTY_STRING = "";
    private Module module;
    private final Uri id;
    private final Identifier dataId;

    public FileItem(Identifier id, Module module) {
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
    public String getTitle() throws Exception {
      return module.getProperty(ModuleAttributes.TITLE, EMPTY_STRING);
    }

    @Override
    public String getAuthor() throws Exception {
      return module.getProperty(ModuleAttributes.AUTHOR, EMPTY_STRING);
    }
    
    @Override
    public String getProgram() throws Exception {
      return module.getProperty(ModuleAttributes.PROGRAM, EMPTY_STRING);
    }
    
    @Override
    public String getComment() throws Exception {
      return module.getProperty(ModuleAttributes.COMMENT, EMPTY_STRING);
    }
    
    @Override
    public String getStrings() throws Exception {
      return module.getProperty(ModuleAttributes.STRINGS, EMPTY_STRING);
    }

    @Override
    public TimeStamp getDuration() throws Exception {
      final long frameDuration = module.getProperty(Properties.Sound.FRAMEDURATION, Properties.Sound.FRAMEDURATION_DEFAULT);
      return TimeStamp.createFrom(frameDuration * module.getDuration(), TimeUnit.MICROSECONDS);
    }

    @Override
    public Module getModule() {
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
