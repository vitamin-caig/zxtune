package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

final class StubCacheDir implements CacheDir {

  @Override
  public File findOrCreate(String... ids) throws IOException {
    throw new IOException("No cache available");
  }

  @Nullable
  @Override
  public ByteBuffer findFile(String... ids) {
    return null;
  }

  @Override
  public Uri createFile(String id, ByteBuffer data) {
    return Uri.EMPTY;
  }

  @Override
  public OutputStream createFile(String id) throws IOException {
    throw new IOException("Should not be called");
  }

  @Override
  public CacheDir createNested(String id) {
    return this;
  }
}
