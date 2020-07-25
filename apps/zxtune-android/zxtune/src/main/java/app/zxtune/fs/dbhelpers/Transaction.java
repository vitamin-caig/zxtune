/**
 * @file
 * @brief Transaction class
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteDatabase;

public abstract class Transaction {

  public static Transaction create(final SQLiteDatabase db) {
    db.beginTransaction();
    return new Transaction() {

      @Override
      public void succeed() {
        db.setTransactionSuccessful();
      }

      @Override
      public void finish() {
        db.endTransaction();
      }
    };
  }

  abstract public void succeed();
  abstract public void finish();
}
