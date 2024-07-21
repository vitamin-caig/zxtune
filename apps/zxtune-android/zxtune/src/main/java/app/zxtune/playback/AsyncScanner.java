package app.zxtune.playback;

import android.net.Uri;
import android.os.SystemClock;

import androidx.annotation.Nullable;
import androidx.core.os.OperationCanceledException;

import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Scanner;
import app.zxtune.fs.VfsFile;

public final class AsyncScanner {

  private static final String TAG = AsyncScanner.class.getName();

  public interface Callback {
    enum Reply {
      RETRY,
      CONTINUE,
    }

    @Nullable
    VfsFile getNextFile();

    Reply onItem(PlayableItem item);

    void onError(Identifier id, Exception e);
  }

  private final ExecutorService executor = Executors.newCachedThreadPool();
  private final AtomicInteger scansDone = new AtomicInteger(0);

  private AsyncScanner() {
  }

  //! @return handle to scan session
  static Object scan(Callback cb) {
    Holder.INSTANCE.post(cb);
    return cb;//use object itself
  }

  private class CallbackWrapper {

    private final WeakReference<Callback> delegate;
    private final int id;

    CallbackWrapper(Callback cb) {
      this.delegate = new WeakReference<>(cb);
      this.id = scansDone.getAndIncrement();
    }

    @Nullable
    final VfsFile getNextFile() {
      return delegate.get().getNextFile();
    }

    final void onItem(PlayableItem item) {
      while (Callback.Reply.RETRY == delegate.get().onItem(item)) {
        waitOrStop();
      }
    }

    private void waitOrStop() {
      final int MAX_SESSIONS_ALIVE = 5;
      if (scansDone.get() - id > MAX_SESSIONS_ALIVE) {
        delegate.clear();
        Log.d(TAG, "Force callback reset");
      } else {
        SystemClock.sleep(500);
      }
    }

    final void onError(Identifier id, Exception e) {
      final Callback cb = delegate.get();
      if (cb != null) {
        cb.onError(id, e);
      } else {
        final RuntimeException ex = new OperationCanceledException("Abandoned error");
        ex.initCause(e);
        throw ex;
      }
    }
  }

  private void post(Callback callback) {
    final CallbackWrapper cb = new CallbackWrapper(callback);
    executor.execute(new Runnable() {
      @Override
      public void run() {
        Log.d(TAG, "Worker(%h): started", this);
        if (scan(cb)) {
          Log.d(TAG, "Worker(%h): finished", this);
        } else {
          Log.d(TAG, "Worker(%h): dead", this);
        }
      }
    });
  }

  private static boolean scan(CallbackWrapper cb) {
    try {
      for (; ; ) {
        final VfsFile file = cb.getNextFile();
        if (file != null) {
          scan(file, cb);
        } else {
          break;
        }
      }
      return true;
    } catch (OperationCanceledException c) {
      // Do not log anything here
      return false;
    } catch (Exception e) {
      Log.w(TAG, e, "Stopped scanning");
      return false;
    }
  }

  private static void scan(VfsFile file, final CallbackWrapper cb) {
    Scanner.analyzeFile(file, new Scanner.Callback() {
      @Override
      public void onModule(Identifier id, Module module) {
        final FileItem item = new FileItem(id, module);
        cb.onItem(item);
      }

      @Override
      public void onError(Identifier id, Exception e) {
        cb.onError(id, e);
      }
    });
  }

  static class FileItem implements PlayableItem {

    private static final String EMPTY_STRING = "";
    private final Uri id;
    private final Identifier dataId;
    private final Module module;

    FileItem(Identifier id, Module module) {
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
      return module.getProperty(ModuleAttributes.TITLE, EMPTY_STRING);
    }

    @Override
    public String getAuthor() {
      return module.getProperty(ModuleAttributes.AUTHOR, EMPTY_STRING);
    }

    @Override
    public String getProgram() {
      return module.getProperty(ModuleAttributes.PROGRAM, EMPTY_STRING);
    }

    @Override
    public String getComment() {
      return module.getProperty(ModuleAttributes.COMMENT, EMPTY_STRING);
    }

    @Override
    public String getStrings() {
      return module.getProperty(ModuleAttributes.STRINGS, EMPTY_STRING);
    }

    @Override
    public TimeStamp getDuration() {
      return module.getDuration();
    }

    @Override
    public long getSize() {
      return module.getProperty(ModuleAttributes.SIZE, 0);
    }

    @Override
    public Module getModule() {
      return module;
    }
  }

  private static class Holder {
    static final AsyncScanner INSTANCE = new AsyncScanner();
  }
}
