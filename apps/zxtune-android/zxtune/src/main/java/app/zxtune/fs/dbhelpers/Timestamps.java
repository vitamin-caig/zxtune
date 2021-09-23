/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDoneException;
import android.database.sqlite.SQLiteStatement;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.room.Dao;
import androidx.room.Entity;
import androidx.room.PrimaryKey;
import androidx.room.Query;

import app.zxtune.TimeStamp;

public class Timestamps {

  private static class Table {
    enum Fields {
      _id, stamp
    }

    //final static String NAME = "timestamps";

    static final String CREATE_QUERY = "CREATE TABLE timestamps (" +
            "_id  TEXT PRIMARY KEY, " +
            "stamp DATETIME NOT NULL" +
            ");";

    static final String QUERY_STATEMENT =
            "SELECT strftime('%s', 'now') - strftime('%s', stamp) FROM timestamps WHERE _id = ?;";

    static final String INSERT_STATEMENT =
            "REPLACE INTO timestamps VALUES (?, CURRENT_TIMESTAMP);";
  }

  public static final String CREATE_QUERY = Table.CREATE_QUERY;

  public interface Lifetime {
    boolean isExpired();

    void update();
  }

  private final SQLiteStatement queryStatement;
  private final SQLiteStatement updateStatement;

  public Timestamps(DBProvider helper) {
    this(helper.getReadableDatabase(), helper.getWritableDatabase());
  }

  @VisibleForTesting
  Timestamps(SQLiteDatabase readable, SQLiteDatabase writable) {
    this.queryStatement = readable.compileStatement(Table.QUERY_STATEMENT);
    this.updateStatement = writable.compileStatement(Table.INSERT_STATEMENT);
  }

  public final Lifetime getLifetime(String id, TimeStamp ttl) {
    return new DbLifetime(id, ttl);
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
        final TimeStamp age = TimeStamp.fromSeconds(queryAge(objId));
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

  // TODO: make the only implementation
  @Dao
  public static abstract class DAO {

    @Entity(tableName = "timestamps")
    public static class Record {
      @PrimaryKey
      @NonNull
      public String id;

      public long stamp;
    }

    @Query("SELECT strftime('%s') - stamp FROM timestamps WHERE id = :id")
    @Nullable
    protected abstract Long queryAge(String id);

    @Query("REPLACE INTO timestamps VALUES (:id, strftime('%s'))")
    protected abstract void touch(String id);

    private class LifetimeImpl implements Lifetime {

      private final String objId;
      private final TimeStamp TTL;

      LifetimeImpl(String id, TimeStamp ttl) {
        this.objId = id;
        this.TTL = ttl;
      }

      @Override
      public boolean isExpired() {
        final Long age = queryAge(objId);
        return age == null || TimeStamp.fromSeconds(age).compareTo(TTL) > 0;
      }

      @Override
      public void update() {
        touch(objId);
      }
    }

    public final Lifetime getLifetime(String id, TimeStamp ttl) {
      return new LifetimeImpl(id, ttl);
    }
  }
}
