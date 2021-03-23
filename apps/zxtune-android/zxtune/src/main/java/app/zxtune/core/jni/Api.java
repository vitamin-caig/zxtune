package app.zxtune.core.jni;

import androidx.annotation.Nullable;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.core.Module;
import app.zxtune.core.PropertiesContainer;
import app.zxtune.core.ResolvingException;
import app.zxtune.utils.ProgressCallback;

/**
 * TODO: make interface and provide by IoC singleton
 */
public abstract class Api {

  /**
   * Loads single module
   *
   * @param data    binary data
   * @param subPath nested data identifier (e.g. file's path in archive)
   * @return instance of Module
   * @throws ResolvingException if file is not found by identifier
   */
  public abstract Module loadModule(ByteBuffer data, String subPath) throws ResolvingException;

  /**
   * Detects all the modules in data
   *
   * @param data     binary data
   * @param callback called on every found module
   * @param progress optional progress (total=100) tracker
   */
  public abstract void detectModules(ByteBuffer data, DetectCallback callback, @Nullable ProgressCallback progress);

  /**
   * Gets all the registered plugins in core
   *
   * @param visitor
   */
  public abstract void enumeratePlugins(Plugins.Visitor visitor);

  /**
   * Gets library options instance
   *
   * @return
   */
  public abstract PropertiesContainer getOptions();

  public static Api instance() {
    if (Holder.INSTANCE.get() == null) {
      new DelayLoadApi(Holder.INSTANCE).initialize(() -> {});
    }
    return Holder.INSTANCE.get();
  }

  public static void load(Runnable cb) {
    new DelayLoadApi(Holder.INSTANCE).initialize(cb);
  }

  private static class Holder {
    static final AtomicReference<Api> INSTANCE =
        new AtomicReference<>();
  }
}
