package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.nio.ByteBuffer;

final class CompositeCacheDir implements CacheDir {

  private final CacheDir delegates[];

  CompositeCacheDir(CacheDir... delegates) {
    this.delegates = delegates;
  }

  @Nullable
  @Override
  public ByteBuffer findFile(String id) {
    for (CacheDir delegate : delegates) {
      final ByteBuffer res = delegate.findFile(id);
      if (res != null) {
        return res;
      }
    }
    return null;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    return delegates[0].createFile(id, data);
  }

  @Override
  public CacheDir createNested(String id) {
    final CacheDir[] nested = new CacheDir[delegates.length];
    for (int idx = 0; idx < delegates.length; ++idx) {
      nested[idx] = delegates[idx].createNested(id);
    }
    return new CompositeCacheDir(nested);
  }
}
