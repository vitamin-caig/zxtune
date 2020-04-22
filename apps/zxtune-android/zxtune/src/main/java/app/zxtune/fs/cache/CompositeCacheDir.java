package app.zxtune.fs.cache;

import android.net.Uri;

import java.io.File;
import java.io.IOException;
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

  @Override
  public File find(String... ids) {
    File result = null;
    for (CacheDir delegate : delegates) {
      final File res = delegate.find(ids);
      if (res != null) {
        if (res.isFile()) {
          return res;
        } else if (result == null) {
          result = res;
        }
      }
    }
    return result;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    return getPrimary().createFile(id, data);
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
