package app.zxtune.core.jni;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import java.nio.ByteBuffer;
import java.util.concurrent.Callable;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.core.Module;
import app.zxtune.core.PropertiesContainer;
import app.zxtune.core.ResolvingException;
import app.zxtune.utils.ProgressCallback;

class DelayLoadApi implements Api {

  private static final String TAG = DelayLoadApi.class.getName();

  private final AtomicReference<Api> ref;
  private final Callable<Api> factory;

  DelayLoadApi(AtomicReference<Api> ref) {
    this(ref, JniApi::new);
  }

  @VisibleForTesting
  DelayLoadApi(AtomicReference<Api> ref, Callable<Api> factory) {
    this.ref = ref;
    this.factory = factory;
  }

  final void initialize(Runnable cb) {
    if (ref.compareAndSet(null, this)) {
      loadAsync(cb);
    } else {
      cb.run();
    }
  }

  private void loadAsync(Runnable cb) {
    new Thread(() -> loadSync(cb), "JniLoad").start();
  }

  private void loadSync(Runnable cb) {
    emplace(factory);
    cb.run();
  }

  private void waitForLoad() {
    Log.d(TAG, "Waiting for native library loading");
    Thread.dumpStack();
    final long startWating = System.currentTimeMillis();
    emplace(() -> {
      while (isUnset()) {
        synchronized(ref) {
          if (isUnset()) {
            ref.wait(1000);
          }
        }
      }
      return ref.get();
    });
    Log.d(TAG, "Finished waiting (%dms)", System.currentTimeMillis() - startWating);
  }

  private boolean isUnset() {
    return ref.compareAndSet(this, this);
  }

  private void emplace(Callable<Api> provider) {
    try {
      set(provider.call());
    } catch (Throwable e) {
      set(new ErrorApi(e));
    }
  }

  private void set(Api api) {
    synchronized(ref) {
      ref.set(api);
      ref.notifyAll();
    }
  }

  @Override
  public Module loadModule(ByteBuffer data, String subPath) throws ResolvingException {
    waitForLoad();
    return ref.get().loadModule(data, subPath);
  }

  @Override
  public void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress) {
    waitForLoad();
    ref.get().detectModules(data, callback, progress);
  }

  @Override
  public void enumeratePlugins(Plugins.Visitor visitor) {
    waitForLoad();
    ref.get().enumeratePlugins(visitor);
  }

  @Override
  public PropertiesContainer getOptions() {
    waitForLoad();
    return ref.get().getOptions();
  }

  static class ErrorApi implements Api {

    private final RuntimeException error;

    ErrorApi(Throwable error) {
      this.error = new RuntimeException(error);
    }

    @Override
    public Module loadModule(ByteBuffer data, String subPath) {
      throw error;
    }

    @Override
    public void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress) {
      throw error;
    }

    @Override
    public void enumeratePlugins(Plugins.Visitor visitor) {
      throw error;
    }

    @Override
    public PropertiesContainer getOptions() {
      throw error;
    }
  }
}
