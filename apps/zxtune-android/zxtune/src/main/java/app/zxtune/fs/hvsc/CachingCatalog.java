/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.hvsc;

import android.support.annotation.NonNull;
import android.text.TextUtils;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import app.zxtune.Analytics;
import app.zxtune.fs.cache.CacheDir;

class CachingCatalog extends Catalog {

  //private final static String TAG = CachingCatalog.class.getName();
  private static final String CACHE_HTML_FILE = File.separator + RemoteCatalog.VERSION + ".html";

  private final Catalog remote;
  private final CacheDir cache;

  public CachingCatalog(Catalog remote, CacheDir cache) {
    this.remote = remote;
    this.cache = cache.createNested("hvsc");

  }

  @Override
  @NonNull
  public ByteBuffer getFileContent(List<String> path) throws IOException {
    final String relPath = TextUtils.join(File.separator, path);
    final ByteBuffer cached = cache.findFile(relPath);
    if (cached != null) {
      sendCacheEvent("file");
      return cached;
    }
    final String relPathHtml = relPath + CACHE_HTML_FILE;
    final ByteBuffer cachedHtml = cache.findFile(relPathHtml);
    //workaround for possible broken cache
    if (cachedHtml != null && isDirContent(cachedHtml)) {
      sendCacheEvent("dir");
      return cachedHtml;
    }
    final ByteBuffer content = remote.getFileContent(path);
    if (isDirContent(content)) {
      sendRemoteEvent("dir");
      cache.createFile(relPathHtml, content);
    } else {
      sendRemoteEvent("file");
      cache.createFile(relPath, content);
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


  private static void sendRemoteEvent(String scope) {
    Analytics.sendVfsRemoteEvent("hvsc", scope);
  }

  private static void sendCacheEvent(String scope) {
    Analytics.sendVfsCacheEvent("hvsc", scope);
  }
}
