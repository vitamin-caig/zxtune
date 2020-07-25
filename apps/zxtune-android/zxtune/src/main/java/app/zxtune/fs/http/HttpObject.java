package app.zxtune.fs.http;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.io.InputStream;

import app.zxtune.TimeStamp;

public interface HttpObject {

    Uri getUri();

    @Nullable
    Long getContentLength();

    @Nullable
    TimeStamp getLastModified();

    InputStream getInput() throws IOException;
}
