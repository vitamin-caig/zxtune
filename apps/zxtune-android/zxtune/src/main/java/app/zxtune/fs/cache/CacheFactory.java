package app.zxtune.fs.cache;

import android.content.Context;

import java.io.File;
import java.io.IOException;

import app.zxtune.Log;

public final class CacheFactory {
  private static final String TAG = CacheFactory.class.getName();

  public static CacheDir create(Context context) {
    final CacheDir externalCache = makeCache(context.getExternalCacheDir());
    final CacheDir internalCache = makeCache(context.getCacheDir());
    final boolean hasExternalCache = externalCache != null;
    final boolean hasInternalCache = internalCache != null;
    if (hasExternalCache && hasInternalCache) {
      return new CompositeCacheDir(externalCache, internalCache);
    } else if (hasExternalCache) {
      return externalCache;
    } else if (hasInternalCache) {
      return internalCache;
    } else {
      Log.w(TAG, new IOException("No cache dirs found"), "No cache");
      return new StubCacheDir();
    }
  }

  public static CacheDir createMinimal(Context context) {
    final CacheDir externalCache = makeCache(context.getExternalCacheDir());
    if (externalCache != null) {
      return externalCache;
    }
    final CacheDir internalCache = makeCache(context.getCacheDir());
    if (internalCache != null) {
      return internalCache;
    }
    return new StubCacheDir();
  }

  private static CacheDir makeCache(File dir) {
    if (dir != null) {
      return new PersistentCacheDir(dir);
    } else {
      return null;
    }
  }
}
