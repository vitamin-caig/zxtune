package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

final class CompositeCacheDir implements CacheDir {

  private final CacheDir delegates[];

  CompositeCacheDir(CacheDir... delegates) {
    this.delegates = delegates;
  }

  @Override
  public File findOrCreate(String... ids) throws IOException {
    File result = null;
    IOException error = null;
    for (CacheDir delegate : delegates) {
      try {
        final File res = delegate.findOrCreate(ids);
        if (res.isFile()) {
          return res;
        } else if (result == null) {
          result = res;
        }
      } catch (IOException e) {
        if (error == null) {
          error = e;
        }
      }
    }
    if (result != null) {
      return result;
    }
    throw error;
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
