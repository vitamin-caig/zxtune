package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.io.InputStream;

import app.zxtune.TimeStamp;

public interface HttpObject {

    @NonNull
    Uri getUri();

    @Nullable
    Long getContentLength();

    @Nullable
    TimeStamp getLastModified();

    @NonNull
    InputStream getInput() throws IOException;
}
