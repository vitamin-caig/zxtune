package app.zxtune.playback;

import android.net.Uri;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Properties;
import app.zxtune.core.Scanner;

import java.lang.ref.WeakReference;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public final class AsyncScanner {

  private static final String TAG = AsyncScanner.class.getName();

  public interface Callback {
    enum Reply {
      RETRY,
      STOP,
      CONTINUE,
      SKIP,
    }

    Reply onUriProcessing(Uri uri);

    Reply onItem(PlayableItem item);

    void onFinish();

    void onError(Exception e);
  }

  private final ExecutorService executor;

  private AsyncScanner() {
    this.executor = Executors.newCachedThreadPool();
  }

  //! @return handle to scan session
  static Object scan(Uri[] uris, Callback cb) {
    Holder.INSTANCE.post(uris, new WeakReference<>(cb));
    return cb;//use object itself
  }

  private void post(final Uri[] uris, final WeakReference<Callback> ref) {
    executor.execute(new Runnable() {
      @Override
      public void run() {
        Log.d(TAG, "Worker(%h): started with %d uris", this, uris.length);
        if (scan(uris, ref)) {
          Log.d(TAG, "Worker(%h): finished", this);
        } else {
          Log.d(TAG, "Worker(%h): dead", this);
        }
      }
    });
  }

  private static boolean scan(Uri[] uris, final WeakReference<Callback> ref) {
    try {
      for (Uri uri : uris) {
        if (!scan(uri, ref)) {
          break;
        }
      }
      ref.get().onFinish();
      return true;
    } catch (NullPointerException e) {
      return false;
    }
  }

  private static boolean scan(Uri uri, final WeakReference<Callback> ref) {
    switch (ref.get().onUriProcessing(uri)) {
      case SKIP:
        return true;
      case STOP:
        return false;
    }
    Scanner.analyzeIdentifier(new Identifier(uri), new Scanner.Callback() {
      @Override
      public void onModule(Identifier id, Module module) throws Exception {
        final FileItem item = new FileItem(id, module);
        try {
          while (Callback.Reply.RETRY == ref.get().onItem(item)) {
            Thread.sleep(500);
          }
        } catch (InterruptedException e) {
          throw new Exception("Interrupted", e);
        }
      }

      @Override
      public void onError(Exception e) {
        final Callback cb = ref.get();
        if (cb != null) {
          cb.onError(e);
        } else {
          Log.w(TAG, e, "Abandoned error");
        }
      }
    });
    return true;
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
  }

  private static class Holder {
    public static final AsyncScanner INSTANCE = new AsyncScanner();
  }
}
