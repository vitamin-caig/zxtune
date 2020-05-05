package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

public class DBProvider {

  private static final String TAG = DBProvider.class.getName();

  private final SQLiteOpenHelper delegate;

  public DBProvider(SQLiteOpenHelper delegate) {
    this.delegate = delegate;
  }

  public final SQLiteDatabase getWritableDatabase() {
    return delegate.getWritableDatabase();
  }

  public final SQLiteDatabase getReadableDatabase() {
    return delegate.getReadableDatabase();
  }
}
