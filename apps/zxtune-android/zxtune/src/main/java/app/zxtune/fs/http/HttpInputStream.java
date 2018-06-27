package app.zxtune.fs.http;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;

final class HttpInputStream extends InputStream {

  private HttpURLConnection connection;
  private InputStream delegate;

  HttpInputStream(HttpURLConnection connection) throws IOException {
    this.connection = connection;
    this.delegate = connection.getInputStream();
  }

  @Override
  public void close() {
    delegate = null;
    if (connection != null) {
      connection.disconnect();
      connection = null;
    }
  }

  @Override
  public int available() throws IOException {
    return delegate.available();
  }

  @Override
  public int read() throws IOException {
    return delegate.read();
  }

  @Override
  public int read(byte[] b, int off, int len) throws IOException {
    return delegate.read(b, off, len);
  }

  @Override
  public int read(byte[] b) throws IOException {
    return delegate.read(b);
  }

  @Override
  public long skip(long n) throws IOException {
    return delegate.skip(n);
  }
}
