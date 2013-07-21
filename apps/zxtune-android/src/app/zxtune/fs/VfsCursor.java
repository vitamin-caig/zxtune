/**
 * @file
 * @brief Implementation of Cursor based on Vfs content
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import android.database.AbstractCursor;

public class VfsCursor extends AbstractCursor {
  
  public static final class Columns {
    public static final int ID = 0;
    public static final int TYPE = 1;
    public static final int NAME = 2;
    public static final int URI = 3;
    public static final int SIZE = 4;
    
    public static final int TOTAL = 5;
  }
  
  public static final class Types {
    public static final int FILE = 0;
    public static final int DIR = 1;
  }

  private Vfs.Entry[] entries;
  private Object[][] resolvedEntries;
  private int resolvedEntriesCount;

  public VfsCursor(Vfs.Dir dir) {
    this.entries = dir.list();
  }
  
  @Override
  public void close() {
    entries = null;
    resolvedEntries = null;
    super.close();
  }

  @Override
  public String[] getColumnNames() {
    //column _id is mandatory for CursorAdapter
    final String[] NAMES = {"_id", "type", "name", "uri", "size"};
    return NAMES;
  }

  @Override
  public int getCount() {
    return resolvedEntries != null ? resolvedEntries.length : entries.length;
  }

  @Override
  public double getDouble(int col) {
    return (Double) getObject(col);
  }

  @Override
  public float getFloat(int col) {
    return (Float) getObject(col);
  }

  @Override
  public int getInt(int col) {
    return (Integer) getObject(col);
  }

  @Override
  public long getLong(int col) {
    return (Long) getObject(col);
  }

  @Override
  public short getShort(int col) {
    return (Short) getObject(col);
  }

  @Override
  public String getString(int col) {
    return (String) getObject(col);
  }

  @Override
  public boolean isNull(int col) {
    return null == getObject(col);
  }
  
  private Object getObject(int col) {
    return getEntry()[col];
  }

  private Object[] getEntry() {
    if (resolvedEntries == null) {
      resolvedEntries = new Object[entries.length][];
    }
    if (null == resolvedEntries[mPos]) {
      resolvedEntries[mPos] = createEntry(mPos, entries[mPos]);
      entries[mPos] = null;
      if (entries.length == ++resolvedEntriesCount) {
        entries = null;
      }
    }
    return resolvedEntries[mPos];
  }
  
  private Object[] createEntry(int id, Vfs.Entry entry) {
    final Object[] res = new Object[Columns.TOTAL];
    res[Columns.ID] = Long.valueOf(id);
    res[Columns.NAME] = entry.name();
    res[Columns.URI] = entry.uri().toString();
    if (entry instanceof Vfs.Dir) {
      res[Columns.TYPE] = Integer.valueOf(Types.DIR);
    } else {
      res[Columns.TYPE] = Integer.valueOf(Types.FILE);
      final Vfs.File asFile = (Vfs.File) entry; 
      res[Columns.SIZE] = Long.valueOf(asFile.size());
    }
    return res;
  }
}
