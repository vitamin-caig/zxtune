package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

final class CompositeCacheDir implements CacheDir {

  private final CacheDir delegates[];

  CompositeCacheDir(CacheDir... delegates) {
    this.delegates = delegates;
  }

  @Nullable
  @Override
  public ByteBuffer findFile(String... ids) {
    for (CacheDir delegate : delegates) {
      final ByteBuffer res = delegate.findFile(ids);
      if (res != null) {
        return res;
      }
    }
    return null;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    return getPrimary().createFile(id, data);
  }

  @Override
  public OutputStream createFile(String id) throws IOException {
    IOException ex = null;
    for (CacheDir sub : delegates) {
      try {
        return sub.createFile(id);
      } catch (IOException e) {
        if (ex == null) {
          ex = e;
        }
      }
    }
    throw ex;
  }

  @Override
  public CacheDir createNested(String id) {
    final CacheDir[] nested = new CacheDir[delegates.length];
    for (int idx = 0; idx < delegates.length; ++idx) {
      nested[idx] = delegates[idx].createNested(id);
    }
    return new CompositeCacheDir(nested);
  }

  private CacheDir getPrimary() {
    return delegates[0];
  }
}
