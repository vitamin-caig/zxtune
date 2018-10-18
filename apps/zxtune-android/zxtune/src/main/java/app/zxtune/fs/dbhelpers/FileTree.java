/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.support.annotation.Nullable;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.nio.ByteBuffer;
import java.util.AbstractMap;
import java.util.HashMap;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.DBProvider;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Objects;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

/**
 * Version 1
 *
 * CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);
 * standard timestamps
 *
 * entries - serialized HashMap<String, String>
 */

final public class FileTree {

  private static final String TAG = FileTree.class.getName();

  private static final int VERSION = 1;

  static final class Table {
    enum Fields {
      _id, entries
    }

    static final String CREATE_QUERY = "CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);";

    private final SQLiteDatabase db;
    private final SQLiteStatement insertStatement;

    Table(DBProvider helper) throws IOException {
      this.db = helper.getWritableDatabase();
      this.insertStatement = db.compileStatement("INSERT OR REPLACE INTO dirs VALUES(?, ?);");
    }

    final void add(String id, byte[] entries) {
      insertStatement.bindString(1, id);
      insertStatement.bindBlob(2, entries);
      insertStatement.executeInsert();
    }

    final byte[] get(String id) {
      final String[] columns = {"entries"};
      final String[] selections = {id};
      final Cursor cursor = db.query("dirs", columns, "_id = ?", selections, null, null, null);
      if (cursor != null) {
        final byte[] res = cursor.moveToFirst() ? cursor.getBlob(0) : null;
        cursor.close();
        return res;
      } else {
        return null;
      }
    }
  }

  private final DBProvider helper;
  private final Table entries;
  private final Timestamps timestamps;

  public FileTree(Context context, String id) throws IOException {
    this.helper = new DBProvider(Helper.create(context, id));
    this.entries = new Table(helper);
    this.timestamps = new Timestamps(helper);
  }

  public final Transaction startTransaction() throws IOException {
    return new Transaction(helper.getWritableDatabase());
  }

  public final Timestamps.Lifetime getDirLifetime(String path, TimeStamp ttl) {
    return timestamps.getLifetime(path, ttl);
  }

  public final void add(String path, HashMap<String, String> obj) throws IOException {
    entries.add(path, serialize(obj));
  }

  public final HashMap<String, String> find(String path) {
    try {
      final byte[] data = entries.get(path);
      return data != null
              ? (HashMap<String, String>) deserialize(data)
              : null;
    } catch (IOException e) {
      return null;
    }
  }

  private static byte[] serialize(Serializable obj) throws IOException {
    final ByteArrayOutputStream stream = new ByteArrayOutputStream(1024);
    final ObjectOutputStream out = new ObjectOutputStream(stream);
    try {
      out.writeObject(obj);
    } finally {
      out.close();
    }
    return stream.toByteArray();
  }

  private static Object deserialize(byte[] data) throws IOException {
    final ByteArrayInputStream stream = new ByteArrayInputStream(data);
    final ObjectInputStream in = new ObjectInputStream(stream);
    try {
      return in.readObject();
    } catch (ClassNotFoundException e) {
      throw new IOException(e);
    } finally {
      in.close();
    }
  }

  private static class Helper extends SQLiteOpenHelper {

    static Helper create(Context context, String name) {
      return new Helper(context, name);
    }

    private Helper(Context context, String name) {
      super(context, name, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      db.execSQL(Table.CREATE_QUERY);
      db.execSQL(Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      Utils.cleanupDb(db);
      onCreate(db);
    }
  }
}
