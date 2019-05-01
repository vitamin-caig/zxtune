package app.zxtune.playback;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.os.OperationCanceledException;
import app.zxtune.core.Identifier;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Properties;
import app.zxtune.core.Scanner;
import app.zxtune.fs.VfsFile;

import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public final class AsyncScanner {

  private static final String TAG = AsyncScanner.class.getName();

  public interface Callback {
    enum Reply {
      RETRY,
      CONTINUE,
    }

    @Nullable
    VfsFile getNextFile();

    Reply onItem(@NonNull PlayableItem item);

    void onError(Identifier id, Exception e);
  }

  private final ExecutorService executor;

  private AsyncScanner() {
    this.executor = Executors.newCachedThreadPool();
  }

  //! @return handle to scan session
  static Object scan(Callback cb) {
    Holder.INSTANCE.post(new WeakReference<>(cb));
    return cb;//use object itself
  }

  private void post(final WeakReference<Callback> ref) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        Log.d(TAG, "Worker(%h): started", this);
        if (scan(ref)) {
          Log.d(TAG, "Worker(%h): finished", this);
        } else {
          Log.d(TAG, "Worker(%h): dead", this);
        }
      }
    });
  }

  private static boolean scan(final WeakReference<Callback> ref) {
    try {
      for (;;) {
        final VfsFile file = ref.get().getNextFile();
        if (file != null) {
          scan(file, ref);
        } else {
          break;
        }
      }
      return true;
    } catch (Exception e) {
      Log.w(TAG, e, "Stopped scanning");
      return false;
    }
  }

  private static void scan(VfsFile file, final WeakReference<Callback> ref) {
    Scanner.analyzeFile(file, new Scanner.Callback() {
      @Override
      public void onModule(Identifier id, Module module) {
        final FileItem item = new FileItem(id, module);
        try {
          while (Callback.Reply.RETRY == ref.get().onItem(item)) {
            Thread.sleep(500);
          }
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Interrupted");
        }
      }

      @Override
      public void onError(Identifier id, Exception e) {
        final Callback cb = ref.get();
        if (cb != null) {
          cb.onError(id, e);
        } else {
          final RuntimeException ex = new OperationCanceledException("Abandoned error");
          ex.initCause(e);
          throw ex;
        }
      }
    });
  }

  static class FileItem implements PlayableItem {

    private static final String EMPTY_STRING = "";
    private final Uri id;
    private final Identifier dataId;
    private Module module;

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
      final long frameDuration = module.getProperty(Properties.Sound.FRAMEDURATION, Properties.Sound.FRAMEDURATION_DEFAULT);
      return TimeStamp.createFrom(frameDuration * module.getDuration(), TimeUnit.MICROSECONDS);
    }

    @Override
    public Module getModule() {
      return module;
    }
  }

  private static class Holder {
    public static final AsyncScanner INSTANCE = new AsyncScanner();
  }
}
