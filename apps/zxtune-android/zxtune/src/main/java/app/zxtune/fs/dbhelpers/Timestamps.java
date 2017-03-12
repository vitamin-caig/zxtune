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

import android.database.sqlite.SQLiteDoneException;
import android.database.sqlite.SQLiteStatement;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;

public class Timestamps {
  
  private static class Table {
    static enum Fields {
      _id, stamp
    }
    
    //final static String NAME = "timestamps";
    
    final static String CREATE_QUERY = "CREATE TABLE timestamps (" +
        "_id  TEXT PRIMARY KEY, " +
        "stamp DATETIME NOT NULL" +
        ");";
    
    final static String QUERY_STATEMENT =
        "SELECT strftime('%s', 'now') - strftime('%s', stamp) FROM timestamps WHERE _id = ?;";
    
    final static String INSERT_STATEMENT =
        "REPLACE INTO timestamps VALUES (?, CURRENT_TIMESTAMP);";
  }
  
  public static String CREATE_QUERY = Table.CREATE_QUERY;
  
  public interface Lifetime {
    boolean isExpired();
    void update();
  }
  
  private final SQLiteStatement queryStatement;
  private final SQLiteStatement updateStatement;
  
  public Timestamps(DBProvider helper) throws IOException {
    this.queryStatement = helper.getReadableDatabase().compileStatement(Table.QUERY_STATEMENT);
    this.updateStatement = helper.getWritableDatabase().compileStatement(Table.INSERT_STATEMENT);
  }
  
  public final Lifetime getLifetime(String id, TimeStamp ttl) {
    return new DbLifetime(id, ttl);
  }
  
  public final Lifetime getStubLifetime() {
    return StubLifetime.INSTANCE;
  }
  
  private long queryAge(String id) {
    synchronized (queryStatement) {
      queryStatement.clearBindings();
      queryStatement.bindString(1 + Table.Fields._id.ordinal(), id);
      return queryStatement.simpleQueryForLong();
    }
  }
  
  private void updateAge(String id) {
    synchronized (updateStatement) {
      updateStatement.clearBindings();
      updateStatement.bindString(1 + Table.Fields._id.ordinal(), id);
      updateStatement.executeInsert();
    }
  }
  
  private class DbLifetime implements Lifetime {
    
    private final String objId;
    private final TimeStamp TTL;
    
    DbLifetime(String id, TimeStamp ttl) {
      this.objId = id;
      this.TTL = ttl;
    }
    
    @Override
    public boolean isExpired() {
      try {
        final TimeStamp age = TimeStamp.createFrom(queryAge(objId), TimeUnit.SECONDS);
        return age.compareTo(TTL) > 0;
      } catch (SQLiteDoneException e) {
        return true;
      }
    }
    
    @Override
    public void update() {
      updateAge(objId);
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
