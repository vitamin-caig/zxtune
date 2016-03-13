/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import android.content.Context;
import android.net.Uri;
import app.zxtune.Log;

public class VfsCache {
  
  private final static String TAG = VfsCache.class.getName();
  private final static int MIN_CACHED_FILE_SIZE = 256;
  
  private final File primary;
  private final File fallback;
  private final boolean hasFallback;

  private VfsCache(File primary, File fallback) {
    if (primary == null && fallback == null) {
      throw new RuntimeException("No cache directories specified");
    }
    this.primary = primary != null ? primary : fallback;
    this.fallback = fallback;
    this.hasFallback = !primary.equals(fallback) && fallback != null;
  }
  
  public static VfsCache create(Context context, String name) {
    return new VfsCache(getExternalDir(context, name), getInternalDir(context, name));
  }
  
  public static VfsCache createExternal(Context context, String name) {
    final File external = getExternalDir(context, name);
    return new VfsCache(external, external);
  }
  
  public final ByteBuffer getCachedFileContent(String path) {
    try {
      ByteBuffer result = readFrom(getPrimaryFile(path));
      if (result == null && hasFallback) {
        result = readFrom(getFallbackFile(path));
      }
      return result;
    } catch (IOException e) {
      Log.d(TAG, e, "Failed to read from cache");
    }
    return null;
  }

  public final void putCachedFileContent(String path, ByteBuffer content) {
    putCachedFileContent(path, content, MIN_CACHED_FILE_SIZE);
  }
  
  public final Uri putAnyCachedFileContent(String path, ByteBuffer content) {
    return putCachedFileContent(path, content, 1);
  }
  
  public final Uri putCachedFileContent(String path, ByteBuffer content, int minSize) {
    if (content.capacity() >= minSize) {
      final File file = getOutputFile(path);
      writeTo(file, content);
      return Uri.fromFile(file);
    } else {
      Log.d(TAG, "Do not cache small content of %s", path);
      return Uri.EMPTY;
    }
  }
  
  private static File getSub(File dir, String name) {
    return dir != null ? new File(dir, name) : null;
  }
  
  private static File getExternalDir(Context ctx, String name) {
    return getSub(ctx.getExternalCacheDir(), name);
  }
  
  private static File getInternalDir(Context ctx, String name) {
    return getSub(ctx.getCacheDir(), name);
  }
  
  private File getPrimaryFile(String path) {
    return getSub(primary, path);
  }
  
  private File getFallbackFile(String path) {
    return getSub(fallback, path); 
  }

  private File getOutputFile(String path) {
    return getPrimaryFile(path);
  }
  
  static ByteBuffer readFrom(File file) throws IOException {
    if (file == null) {
      return null;
    }
    if (!file.isFile()) {
      Log.d(TAG, "No cached file " + file.getAbsolutePath());
      return null;
    }
    Log.d(TAG, "Reading cached file %s", file.getAbsolutePath());
    final FileInputStream stream = new FileInputStream(file);
    try {
      final FileChannel channel = stream.getChannel();
      try {
        return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
      } finally {
        channel.close();
      }
    } finally {
      stream.close();
    }
  }

  static void writeTo(File file, ByteBuffer data) {
    try {
      Log.d(TAG, "Write cached file %s", file.getAbsolutePath());
      file.getParentFile().mkdirs();
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        final FileChannel chan = stream.getChannel();
        try {
          chan.write(data);
        } finally {
          data.position(0);
          chan.close();
        }
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, e, "Failed to write to %s", file.getAbsolutePath());
      file.delete();
    }
  }
}
