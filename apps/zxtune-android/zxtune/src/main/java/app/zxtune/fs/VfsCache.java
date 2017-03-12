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

import android.content.Context;
import android.net.Uri;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import app.zxtune.Log;

public class VfsCache {
  
  private final static String TAG = VfsCache.class.getName();
  private final static int MIN_CACHED_FILE_SIZE = 256;

  private final File dir;

  private VfsCache(File dir) {
    this.dir = dir;
  }
  
  public static VfsCache create(Context context) {
    final File externalCache = context.getExternalCacheDir();
    if (externalCache != null) {
      return new VfsCache(externalCache);
    }
    final File internalCache = context.getCacheDir();
    if (internalCache != null) {
      return new VfsCache(internalCache);
    }
    Log.w(TAG, new IOException("No cache dirs found"), "No cache");
    return new VfsCache(null);
  }

  public final VfsCache createNested(String name) {
    return new VfsCache(getSub(name));
  }
  
  public final ByteBuffer getCachedFileContent(String path) {
    try {
      final File file = getSub(path);
      if (file == null || !file.isFile()) {
        Log.d(TAG, "No cached file %s", file.getAbsolutePath());
        return null;
      }
      Log.d(TAG, "Reading cached file %s", file.getAbsolutePath());
      return readFrom(file);
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to read from cache");
    }
    return null;
  }

  public final void putCachedFileContent(String path, ByteBuffer content) {
    if (content.capacity() >= MIN_CACHED_FILE_SIZE) {
      putAnyCachedFileContent(path, content);
    } else {
      Log.d(TAG, "Do not cache small content of %s", path);
    }
  }
  
  public final Uri putAnyCachedFileContent(String path, ByteBuffer content) {
    final File file = getSub(path);
    if (file != null) {
      Log.d(TAG, "Write cached file %s", file.getAbsolutePath());
      try {
        file.getParentFile().mkdirs();
        writeTo(file, content);
        return Uri.fromFile(file);
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to write to %s", file.getAbsolutePath());
        file.delete();
      }
    }
    return Uri.EMPTY;
  }
  
  private File getSub(String path) {
    return dir != null ? new File(dir, path) : null;
  }

  static ByteBuffer readFrom(File file) throws IOException {
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
        Log.w(TAG, e, "Failed to read using MMAP. Use fallback");
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

  private static void writeTo(File file, ByteBuffer data) throws IOException {
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
  }
}
