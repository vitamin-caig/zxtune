package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;

import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

class FileOperation implements AsyncQueryOperation {

  private static final String[] COLUMNS = {
      OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE};

  private final Uri uri;
  private final Resolver resolver;
  @Nullable
  private final String[] projection;

  FileOperation(Uri uri, Resolver resolver, @Nullable String[] projection) {
    this.uri = uri;
    this.resolver = resolver;
    this.projection = projection;
  }

  @Nullable
  @Override
  public Cursor call() throws Exception {
    final VfsFile file = maybeResolve();
    if (file == null) {
      return null;
    }
    final File f = maybeGetFile(file);
    if (f != null) {
      return makeResult(file, f);
    } else {
      return null;
    }
  }

  @Nullable
  private VfsFile maybeResolve() throws IOException {
    final VfsObject obj = resolver.resolve(uri);
    if (obj instanceof VfsFile) {
      return (VfsFile) obj;
    }
    return null;
  }

  @Nullable
  private File maybeGetFile(VfsFile file) {
    final File res = Vfs.getCacheOrFile(file);
    if (res != null && res.isFile()) {
      return res;
    }
    return null;
  }

  @Nullable
  @Override
  public Cursor status() {
    return null;
  }

  final ParcelFileDescriptor openFile(String mode) throws Exception {
    if (!"r".equals(mode)) {
      throw new IllegalArgumentException("Invalid mode: " + mode);
    }
    final VfsFile file = maybeResolve();
    if (file == null) {
      throw new IOException("Failed to resolve " + uri);
    }
    final File f = maybeGetFile(file);
    if (f == null) {
      throw new IOException("Failed to get file content of " + uri);
    }
    return ParcelFileDescriptor.open(f, ParcelFileDescriptor.MODE_READ_ONLY);
  }

  // as in FileProvider
  private Cursor makeResult(VfsFile file, File content) {
    final String[] columns = projection != null ? projection : COLUMNS;
    String[] cols = new String[columns.length];
    Object[] values = new Object[columns.length];
    int i = 0;
    for (String col : columns) {
      if (OpenableColumns.DISPLAY_NAME.equals(col)) {
        cols[i] = OpenableColumns.DISPLAY_NAME;
        values[i] = file.getName();
        ++i;
      } else if (OpenableColumns.SIZE.equals(col)) {
        cols[i] = OpenableColumns.SIZE;
        values[i] = content.length();
        ++i;
      }
    }

    cols = copyOf(cols, i);
    values = copyOf(values, i);

    final MatrixCursor cursor = new MatrixCursor(cols, 1);
    cursor.addRow(values);
    return cursor;
  }

  private static String[] copyOf(String[] original, int newLength) {
    final String[] result = new String[newLength];
    System.arraycopy(original, 0, result, 0, newLength);
    return result;
  }


  private static Object[] copyOf(Object[] original, int newLength) {
    final Object[] result = new Object[newLength];
    System.arraycopy(original, 0, result, 0, newLength);
    return result;
  }
}
