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
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.util.Log;
import app.zxtune.R;

public class VfsCache {
  
  private final static String TAG = VfsCache.class.getName();
  private final static int MIN_CACHED_FILE_SIZE = 256;
  
  private final String cacheDir;

  public VfsCache(Context ctx, String name) {
    this.cacheDir = getCacheDir(ctx, name);
    createCacheDir();
  }
  
  public final File getCachedFile(String path) {
    return new File(cacheDir + path);
  }
  
  public final ByteBuffer getCachedFileContent(String path) {
    final File file = getCachedFile(path);
    try {
      if (file.exists() && file.isFile()) {
        Log.d(TAG, "Reading cached file " + file.getAbsolutePath());
        return readFrom(file);
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to read from cache", e);
    }
    return null;
  }

  public final void putCachedFileContent(String path, ByteBuffer content) {
    putCachedFileContent(path,  content, MIN_CACHED_FILE_SIZE);
  }
  
  public final void putAnyCachedFileContent(String path, ByteBuffer content) {
    putCachedFileContent(path,  content, 0);
  }
  
  public final void putCachedFileContent(String path, ByteBuffer content, int minSize) {
    if (content.capacity() >= minSize) {
      final File file = getCachedFile(path);
      Log.d(TAG, "Write cached file " + file.getAbsolutePath());
      writeTo(file, content);
    } else {
      Log.d(TAG, "Do not cache small content of " + path);
    }
    
  }

  static ByteBuffer readFrom(File file) throws IOException {
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
      file.getParentFile().mkdirs();
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        final FileChannel chan = stream.getChannel();
        try {
          chan.write(data);
        } finally {
          chan.close();
        }
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to write to " + file.getAbsolutePath(), e);
      file.delete();
    }
  }
  
  private void createCacheDir() {
    final File dir = new File(cacheDir);
    if (!dir.exists()) {
      Log.d(TAG, "Create cache dir " + cacheDir);
      dir.mkdirs();
    } else {
      Log.d(TAG, "Use existing cache dir " + cacheDir);
    }
  }

  private static String getCacheDir(Context context, String name) {
    final boolean useExternalDir = getUseExternalDir(context);
    final File cacheDir = (useExternalDir ? context.getExternalCacheDir() : context.getCacheDir());
    return cacheDir.getAbsolutePath() + File.separator + name + File.separator;
  }
  
  private static boolean getUseExternalDir(Context context) {
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
    final Resources res = context.getResources();
    final String key = res.getString(R.string.pref_vfs_cache_external);
    final boolean defValue = res.getBoolean(R.bool.pref_vfs_cache_external_default);
    return prefs.getBoolean(key, defValue);
  }
}
