package app.zxtune.fs.http;

import android.net.Uri;
import androidx.annotation.NonNull;

import java.io.IOException;
import java.io.InputStream;

public interface HttpProvider {

    boolean hasConnection();

    @NonNull
    HttpObject getObject(Uri uri) throws IOException;

    @NonNull
    InputStream getInputStream(Uri uri) throws IOException;
}
