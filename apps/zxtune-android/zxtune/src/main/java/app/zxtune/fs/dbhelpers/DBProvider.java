package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;

import java.io.IOException;

import app.zxtune.Log;

// Ugly crutch against simultaneous db access
public class DBProvider {

  private static final String TAG = DBProvider.class.getName();
  private static final int MAX_RETRIES = 10;

  private final SQLiteOpenHelper delegate;

  public DBProvider(SQLiteOpenHelper delegate) {
    this.delegate = delegate;
  }

  public final SQLiteDatabase getWritableDatabase() throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        return delegate.getWritableDatabase();
      } catch (SQLiteException e) {
        if (retry > MAX_RETRIES) {
          throw new IOException(e);
        }
        Log.w(TAG, e, "Failed to get writable database");
      }
      try {
        Thread.sleep(500 + (long) (Math.random() * 500), 0);
      } catch (InterruptedException e) {
        --retry;
      }
    }
  }

  public final SQLiteDatabase getReadableDatabase() {
    return delegate.getReadableDatabase();
  }
}
