package app.zxtune.fs.cache;

import android.net.Uri;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

final class StubCacheDir implements CacheDir {

  @Override
  public File findOrCreate(String... ids) throws IOException {
    throw new IOException("No cache available");
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
