package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;

import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

class FileOperation implements AsyncQueryOperation {

  private static final String[] COLUMNS = {
      OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE };

  private final String[] projection;
  private final Uri uri;
  private VfsFile file;

  FileOperation(@Nullable String[] projection, @NonNull Uri uri) {
    this.projection = projection;
    this.uri = uri;
  }

  FileOperation(@Nullable String[] projection, @NonNull VfsFile file) {
    this.projection = projection;
    this.uri = file.getUri();
    this.file = file;
  }

  @Override
  public Cursor call() throws Exception {
    final File f = maybeGetFile();
    if (f != null) {
      return makeResult(f);
    } else {
      return null;
    }
  }

  @Nullable
  private File maybeGetFile() throws Exception {
    if (file == null) {
      final VfsObject obj = VfsArchive.resolve(uri);
      if (obj instanceof VfsFile) {
        file = (VfsFile) obj;
      }
    }
    final File res = Vfs.getCacheOrFile(file);
    return res != null && res.isFile() ? res : null;
  }

  @Override
  public Cursor status() {
    return null;
  }

  final ParcelFileDescriptor openFile(@NonNull String mode) throws Exception {
    final File f = maybeGetFile();
    if (f == null) {
      throw new IOException("Failed to get file content of " + uri);
    } else if (!"r".equals(mode)) {
      throw new IllegalArgumentException("Invalid mode: " + mode);
    }
    return ParcelFileDescriptor.open(f, ParcelFileDescriptor.MODE_READ_ONLY);
  }

  // as in FileProvider
  private Cursor makeResult(@NonNull File content) {
    final String[] columns = projection != null ? projection : COLUMNS;
    String[] cols = new String[columns.length];
    Object[] values = new Object[columns.length];
    int i = 0;
    for (String col : columns) {
      if (OpenableColumns.DISPLAY_NAME.equals(col)) {
        cols[i] = OpenableColumns.DISPLAY_NAME;
        values[i++] = file.getName();
      } else if (OpenableColumns.SIZE.equals(col)) {
        cols[i] = OpenableColumns.SIZE;
        values[i++] = content.length();
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
