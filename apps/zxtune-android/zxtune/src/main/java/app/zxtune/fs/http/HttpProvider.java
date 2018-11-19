package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

public interface HttpProvider {

    boolean hasConnection();

    @NonNull
    HttpObject getObject(Uri uri) throws IOException;

    @NonNull
    InputStream getInputStream(Uri uri) throws IOException;

    @NonNull
    ByteBuffer getContent(Uri uri) throws IOException;

    void getContent(Uri uri, OutputStream output) throws IOException;

    @NonNull
    String getHtml(Uri uri) throws IOException;
}
