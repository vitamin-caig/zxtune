package app.zxtune.fs.http;

import android.net.Uri;
import android.support.annotation.NonNull;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import app.zxtune.Log;

final class OomRetryingAdapter implements HttpProvider {

  private static final String TAG = OomRetryingAdapter.class.getName();

  private final HttpProvider delegate;

  OomRetryingAdapter(HttpProvider delegate) {
    this.delegate = delegate;
  }

  @Override
  public boolean hasConnection() {
    return delegate.hasConnection();
  }

  @NonNull
  @Override
  public InputStream getInputStream(Uri uri) throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        return delegate.getInputStream(uri);
      } catch (OutOfMemoryError err) {
          if (retry == 1) {
            Log.d(TAG, "Retry getInputStream call for OOM");
            System.gc();
          } else {
            throw new IOException(err);
          }
      }
    }
  }

  @NonNull
  @Override
  public ByteBuffer getContent(Uri uri) throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        return delegate.getContent(uri);
      } catch (OutOfMemoryError err) {
        if (retry == 1) {
          Log.d(TAG, "Retry getContent call for OOM");
          System.gc();
        } else {
          throw new IOException(err);
        }
      }
    }
  }

  @Override
  public void getContent(Uri uri, OutputStream output) throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        delegate.getContent(uri, output);
        return;
      } catch (OutOfMemoryError err) {
        if (retry == 1) {
          Log.d(TAG, "Retry getContent call for OOM");
          System.gc();
        } else {
          throw new IOException(err);
        }
      }
    }
  }

  @NonNull
  @Override
  public String getHtml(Uri uri) throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        return delegate.getHtml(uri);
      } catch (OutOfMemoryError err) {
        if (retry == 1) {
          Log.d(TAG, "Retry getHtml call for OOM");
          System.gc();
        } else {
          throw new IOException(err);
        }
      }
    }
  }
}
