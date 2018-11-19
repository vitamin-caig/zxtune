package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import app.zxtune.TimeStamp;

public interface HttpObject {

    @NonNull
    Uri getUri();

    @Nullable
    Long getContentLength();

    @Nullable
    TimeStamp getLastModified();
}
