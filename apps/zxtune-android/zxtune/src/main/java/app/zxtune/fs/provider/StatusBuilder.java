package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;

class StatusBuilder {
  static Cursor makeError(Exception e) {
    final MatrixCursor result = new MatrixCursor(Schema.Status.COLUMNS, 1);
    result.addRow(Schema.Status.makeError(e));
    return result;
  }

  static Cursor makeProgress(int done) {
    final MatrixCursor result = new MatrixCursor(Schema.Status.COLUMNS, 1);
    result.addRow(Schema.Status.makeProgress(done));
    return result;
  }

  static Cursor makeProgress(int done, int total) {
    final MatrixCursor result = new MatrixCursor(Schema.Status.COLUMNS, 1);
    result.addRow(Schema.Status.makeProgress(done, total));
    return result;
  }
}
