package app.zxtune.fs.cache;

import android.net.Uri;
import android.support.annotation.Nullable;

import java.nio.ByteBuffer;

public interface CacheDir {

    @Nullable
    ByteBuffer findFile(String id);

    Uri createFile(String id, ByteBuffer data);

    CacheDir createNested(String id);
}
