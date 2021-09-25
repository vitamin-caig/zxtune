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

import androidx.annotation.Nullable;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import app.zxtune.Log;
import app.zxtune.TimeStamp;

/**
 * Version 1
 *
 * CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);
 * standard timestamps
 *
 * entries - serialized HashMap<String, String>
 *
 * Version 2
 *
 * Changed serialization format of blob
 */

public class FileTree {

  private static final String TAG = FileTree.class.getName();

  private static final int VERSION = 2;

  static final class Table {
    enum Fields {
      _id, entries
    }

    static final String CREATE_QUERY = "CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);";

    private final SQLiteDatabase db;
    private final SQLiteStatement insertStatement;

    Table(DBProvider helper) {
      this.db = helper.getWritableDatabase();
      this.insertStatement = db.compileStatement("INSERT OR REPLACE INTO dirs VALUES(?, ?);");
    }

    final void add(String id, byte[] entries) {
      insertStatement.bindString(1, id);
      insertStatement.bindBlob(2, entries);
      insertStatement.executeInsert();
    }

    @Nullable
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

  public FileTree(Context context, String id) {
    this.helper = new DBProvider(Helper.create(context, id));
    this.entries = new Table(helper);
    this.timestamps = new Timestamps(helper);
  }

  final void close() {
    helper.close();
  }

  public void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(helper, cmd);
  }

  public Timestamps.Lifetime getDirLifetime(String path, TimeStamp ttl) {
    return timestamps.getLifetime(path, ttl);
  }

  public static class Entry {
    public String name;
    public String descr;
    public String size;

    Entry(DataInput in) throws IOException {
      this.name = in.readUTF();
      this.descr = in.readUTF();
      this.size = in.readUTF();
    }

    final void save(DataOutput out) throws IOException {
      out.writeUTF(name);
      out.writeUTF(descr);
      out.writeUTF(size);
    }

    public Entry(String name, String descr, @Nullable String size) {
      this.name = name;
      this.descr = descr;
      this.size = size != null ? size : "";
    }

    final public boolean isDir() {
      return size.isEmpty();
    }

    @Override
    public boolean equals(Object o) {
      final Entry rh = (Entry) o;
      return rh != null && name.equals(rh.name) && descr.equals(rh.descr) && size.equals(rh.size);
    }
  }

  public void add(String path, List<Entry> obj) throws IOException {
    entries.add(path, serialize(obj));
  }

  @Nullable
  public List<Entry> find(String path) {
    try {
      final byte[] data = entries.get(path);
      return data != null
              ? deserialize(data)
              : null;
    } catch (IOException e) {
      return null;
    }
  }

  private static byte[] serialize(List<Entry> obj) throws IOException {
    final ByteArrayOutputStream stream = new ByteArrayOutputStream(1024);
    final DataOutputStream out = new DataOutputStream(stream);
    try {
      out.writeInt(obj.size());
      for (Entry e : obj) {
        e.save(out);
      }
    } finally {
      out.close();
    }
    return stream.toByteArray();
  }

  private static List<Entry> deserialize(byte[] data) throws IOException {
    final ByteArrayInputStream stream = new ByteArrayInputStream(data);
    final DataInputStream in = new DataInputStream(stream);
    try {
      int count = in.readInt();
      final ArrayList<Entry> result = new ArrayList<>(count);
      while (count-- != 0) {
        result.add(new Entry(in));
      }
      return result;
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
