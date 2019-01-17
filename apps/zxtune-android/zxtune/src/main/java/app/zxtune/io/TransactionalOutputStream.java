package app.zxtune.io;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import app.zxtune.Log;

public class TransactionalOutputStream extends OutputStream {
  private static final String TAG = TransactionalOutputStream.class.getName();

  private final File target;
  private final File temporary;
  private OutputStream delegate;
  private boolean confirmed = false;

  public TransactionalOutputStream(File target) throws IOException {
    Log.d(TAG, "Write cached file %s", target.getAbsolutePath());
    this.target = target;
    this.temporary = new File(target.getPath() + "~" + Integer.toString(hashCode()));
    if (target.getParentFile().mkdirs()) {
      Log.d(TAG, "Created cache dir");
    }
    delegate = new FileOutputStream(temporary);
  }

  @Override
  public void close() throws IOException {
    delegate.close();
    delegate = null;
    if (confirmed) {
      if (!Io.rename(temporary, target)) {
        final String msg = "Failed to rename " + temporary + " to " + target;
        throw temporary.delete()
          ? new IOException(msg)
          : new IOException(msg, new IOException("Failed to delete temporary " + temporary));
      }
    } else if (!temporary.delete()) {
      throw new IOException("Failed to cleanup unconfirmed " + temporary);
    }
  }

  @Override
  public void flush() throws IOException {
    try {
      delegate.flush();
      confirmed = true;
    } catch (IOException e) {
      confirmed = false;
      throw e;
    }
  }

  @Override
  public void write(byte[] b) throws IOException {
    try {
      delegate.write(b);
    } catch (IOException e) {
      confirmed = false;
      throw e;
    }
  }

  @Override
  public void write(byte[] b, int off, int len) throws IOException {
    try {
      delegate.write(b, off, len);
    } catch (IOException e) {
      confirmed = false;
      throw e;
    }
  }

  @Override
  public void write(int b) throws IOException {
    try {
      delegate.write(b);
    } catch (IOException e) {
      confirmed = false;
      throw e;
    }
  }
}
