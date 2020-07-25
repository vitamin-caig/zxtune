package app.zxtune.fs.http;

import android.net.Uri;

import java.io.IOException;
import java.io.InputStream;

public interface HttpProvider {

    boolean hasConnection();

    HttpObject getObject(Uri uri) throws IOException;

    InputStream getInputStream(Uri uri) throws IOException;
}
