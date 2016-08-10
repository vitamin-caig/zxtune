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

  private final File dir;

  private VfsCache(File dir) {
    if (dir == null) {
      throw new RuntimeException("No cache directory specified");
    }
    this.dir = dir;
  }
  
  public static VfsCache create(Context context) {
    final File externalCache = context.getExternalCacheDir();
    if (externalCache != null) {
      return new VfsCache(externalCache);
    }
    final File internalCache = context.getCacheDir();
    return new VfsCache(internalCache);
  }

  public final VfsCache createNested(String name) {
    return new VfsCache(getSub(dir, name));
  }
  
  public final ByteBuffer getCachedFileContent(String path) {
    try {
      return readFrom(getFile(path));
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
      final File file = getFile(path);
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
  
  private File getFile(String path) {
    return getSub(dir, path);
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
        return readFrom(channel);
      } finally {
        channel.close();
      }
    } finally {
      stream.close();
    }
  }

  private static ByteBuffer readFrom(FileChannel channel) throws IOException {
    final int MIN_MMAPED_FILE_SIZE = 131072;
    if (channel.size() >= MIN_MMAPED_FILE_SIZE) {
      try {
        return readMemoryMapped(channel);
      } catch (IOException e) {
        Log.d(TAG, e, "Failed to read using MMAP. Use fallback");
        //http://stackoverflow.com/questions/8553158/prevent-outofmemory-when-using-java-nio-mappedbytebuffer
        System.gc();
        System.runFinalization();
      }
    }
    return readDirectArray(channel);
  }

  private static ByteBuffer readMemoryMapped(FileChannel channel) throws IOException {
      return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
  }

  private static ByteBuffer readDirectArray(FileChannel channel) throws IOException {
    final ByteBuffer direct = ByteBuffer.allocateDirect((int) channel.size());
    channel.read(direct);
    direct.position(0);
    return direct;
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
