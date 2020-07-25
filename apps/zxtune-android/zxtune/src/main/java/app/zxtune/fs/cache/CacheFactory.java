package app.zxtune.fs.cache;

import android.content.Context;

import androidx.annotation.Nullable;

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

  @Nullable
  private static CacheDir makeCache(@Nullable File dir) {
    if (dir != null) {
      return new PersistentCacheDir(dir);
    } else {
      return null;
    }
  }
}
