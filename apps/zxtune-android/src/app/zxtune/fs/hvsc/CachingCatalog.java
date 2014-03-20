/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.hvsc;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.List;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();
  private final static String CACHE_DIR_NAME = "hvsc";
  private final static String CACHE_HTML_FILE = File.separator + ".html";
  private final static int MIN_CACHED_FILE_SIZE = 256;

  private final String cacheDir;
  private final Catalog remote;
  
  public CachingCatalog(Context context, Catalog remote) {
    this.cacheDir = context.getCacheDir().getAbsolutePath() + File.separator + CACHE_DIR_NAME + File.separator;
    this.remote = remote;
    createCacheDir();
  }

  private void createCacheDir() {
    final File dir = new File(cacheDir);
    if (!dir.exists()) {
      dir.mkdirs();
    }
  }

  @Override
  public ByteBuffer getFileContent(List<String> path) throws IOException {
    final String relPath = TextUtils.join(File.separator, path);
    final String relPathHtml = relPath + CACHE_HTML_FILE;
    final File cache = new File(cacheDir + relPath);
    final File cacheHtml = new File(cacheDir + relPathHtml);
    try {
      if (cache.exists() && cache.isFile()) {
        Log.d(TAG, "Reading content of file " + relPath + " from cache");
        return readFrom(cache);
      } else if (cacheHtml.exists()) {
        Log.d(TAG, "Reading content of html page " + relPath + " from cache");
        return readFrom(cacheHtml);
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to read from cache", e);
    }
    final ByteBuffer content = remote.getFileContent(path);
    if (content.capacity() >= MIN_CACHED_FILE_SIZE) {
      if (isDirContent(content)) {
        Log.d(TAG, "Write content of html page " + relPath + " to cache");
        writeTo(cacheHtml, content);
      } else {
        Log.d(TAG, "Write content of file " + relPath + " to cache");
        writeTo(cache, content);
      }
    } else {
      Log.d(TAG, "Do not cache suspicious file");
    }
    return content;
  }

  //TODO: remove C&P
  private static ByteBuffer readFrom(File file) throws IOException {
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

  private static void writeTo(File file, ByteBuffer data) {
    try {
      file.getParentFile().mkdirs();
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        final FileChannel chan = stream.getChannel();
        try {
          data.position(0);
          chan.write(data);
        } finally {
          chan.close();
        }
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "writeTo " + file.getAbsolutePath(), e);
      file.delete();
    }
  }
  
  @Override
  public boolean isDirContent(ByteBuffer buffer) {
    return remote.isDirContent(buffer);
  }

  @Override
  public void parseDir(ByteBuffer data, DirVisitor visitor) throws IOException {
    remote.parseDir(data, visitor);
  }
}
