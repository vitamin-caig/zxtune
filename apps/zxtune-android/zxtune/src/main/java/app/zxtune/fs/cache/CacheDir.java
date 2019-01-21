package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

public interface CacheDir {

  @NonNull
  File findOrCreate(String... ids) throws IOException;

  Uri createFile(String id, ByteBuffer data);

  CacheDir createNested(String id);
}
