package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

public interface CacheDir {

  @Nullable
  ByteBuffer findFile(String... ids);

  Uri createFile(String id, ByteBuffer data);

  OutputStream createFile(String id) throws IOException;

  CacheDir createNested(String id);
}
