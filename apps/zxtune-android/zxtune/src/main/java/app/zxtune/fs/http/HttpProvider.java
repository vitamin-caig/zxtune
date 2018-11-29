package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public interface HttpProvider {

    boolean hasConnection();

    @NonNull
    HttpObject getObject(Uri uri) throws IOException;

    @NonNull
    InputStream getInputStream(Uri uri) throws IOException;
}
