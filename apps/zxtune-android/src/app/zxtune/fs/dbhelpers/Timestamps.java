/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.dbhelpers;

import java.util.concurrent.TimeUnit;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import app.zxtune.TimeStamp;

public class Timestamps {
  
  private static class Table {
    static enum Fields {
      _id, stamp
    }
    
    final static String NAME = "timestamps";
    
    final static String CREATE_QUERY = "CREATE TABLE timestamps (" +
        "_id  TEXT PRIMARY KEY, " +
        "stamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL" +
        ");";
  }
  
  public static String CREATE_QUERY = Table.CREATE_QUERY;
  
  public interface Lifetime {
    boolean isExpired();
    void update();
  }
  
  private final SQLiteOpenHelper helper;
  
  public Timestamps(SQLiteOpenHelper helper) {
    this.helper = helper;
  }
  
  public final Lifetime getLifetime(String id, TimeStamp ttl) {
    return new DbLifetime(helper, id, ttl);
  }
  
  public final Lifetime getStubLifetime() {
    return StubLifetime.INSTANCE;
  }

  static class DbLifetime implements Lifetime {
    
    private final SQLiteOpenHelper helper;
    private final String objId;
    private final TimeStamp TTL;
    
    DbLifetime(SQLiteOpenHelper helper, String id, TimeStamp ttl) {
      this.helper = helper;
      this.objId = id;
      this.TTL = ttl;
    }
    
    @Override
    public boolean isExpired() {
      final SQLiteDatabase db = helper.getReadableDatabase();
      final String selection = Table.Fields._id + " = '" + objId + "'";
      final String target = "strftime('%s', 'now') - strftime('%s', " + Table.Fields.stamp + ")";
      final Cursor cursor = db.query(Table.NAME, new String[] {target}, selection, 
          null, null, null, null, null);
      try {
        if (cursor.moveToFirst()) {
            final TimeStamp age = TimeStamp.createFrom(cursor.getInt(0), TimeUnit.SECONDS);
            return age.compareTo(TTL) > 0;
        }
      } finally {
        cursor.close();
      }
      return true;
    }
    
    @Override
    public void update() {
      final ContentValues values = new ContentValues();
      values.put(Table.Fields._id.name(), objId);
      final SQLiteDatabase db = helper.getWritableDatabase();
      db.insertWithOnConflict(Table.NAME, null/* nullColumnHack */, values,
          SQLiteDatabase.CONFLICT_REPLACE);
    }
  }
  
  static class StubLifetime implements Lifetime {
    
    static final StubLifetime INSTANCE = new StubLifetime();
    
    @Override
    public boolean isExpired() {
      return false;
    }
    
    @Override
    public void update() {}
  }
}
