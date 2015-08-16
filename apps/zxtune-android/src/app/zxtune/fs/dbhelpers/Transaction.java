/**
 * 
 * @file
 *
 * @brief Transaction class
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteDatabase;

public class Transaction {

  private final SQLiteDatabase db;

  public Transaction(SQLiteDatabase db) {
    this.db = db;
    db.beginTransaction();
  }

  public final void succeed() {
    db.setTransactionSuccessful();
  }

  public final void finish() {
    db.endTransaction();
  }
}
