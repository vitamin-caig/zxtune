package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.nio.ByteBuffer;

final class StubCacheDir implements CacheDir {

  @Nullable
  @Override
  public ByteBuffer findFile(String id) {
    return null;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    return Uri.EMPTY;
  }

  @Override
  public CacheDir createNested(String id) {
    return this;
  }
}
