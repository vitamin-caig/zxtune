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
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import android.content.Context;
import android.text.TextUtils;
import app.zxtune.fs.VfsCache;

class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();
  private final static String CACHE_DIR_NAME = "hvsc";
  private final static String CACHE_HTML_FILE = File.separator + RemoteCatalog.VERSION + ".html";

  private final VfsCache cacheDir;
  private final Catalog remote;
  
  public CachingCatalog(Context context, Catalog remote) {
    this.cacheDir = VfsCache.create(context, CACHE_DIR_NAME);
    this.remote = remote;
  }

  @Override
  public ByteBuffer getFileContent(List<String> path) throws IOException {
    final String relPath = TextUtils.join(File.separator, path);
    final ByteBuffer cache = cacheDir.getCachedFileContent(relPath);
    if (cache != null) {
      return cache;
    }
    final String relPathHtml = relPath + CACHE_HTML_FILE;
    final ByteBuffer cacheHtml = cacheDir.getCachedFileContent(relPathHtml);
    //workaround for possible broken cache
    if (cacheHtml != null && isDirContent(cacheHtml)) {
      return cacheHtml;
    }
    final ByteBuffer content = remote.getFileContent(path);
    if (isDirContent(content)) {
      cacheDir.putAnyCachedFileContent(relPathHtml, content);
    } else {
      cacheDir.putCachedFileContent(relPath, content);
    }
    return content;
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
