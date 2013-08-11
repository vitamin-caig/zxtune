/**
 * @file
 * @brief Implementation of Cursor based on Vfs content
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.util.Arrays;
import java.util.Comparator;

import android.database.AbstractCursor;

class VfsCursor extends AbstractCursor {
  
  private Vfs.Entry[] entries;
  private Object[][] resolvedEntries;
  private int resolvedEntriesCount;

  public VfsCursor(Vfs.Dir dir) {
    final Vfs.Entry[] entries = dir.list();
    if (entries != null) {
      Arrays.sort(entries, new CompareEntries());
      this.entries = entries;
    } else {
      this.entries = new Vfs.Entry[0];
    }
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
    return entries != null ? entries.length : resolvedEntriesCount;
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
    final Object[] res = new Object[VfsQuery.Columns.TOTAL];
    res[VfsQuery.Columns.ID] = Long.valueOf(id);
    res[VfsQuery.Columns.NAME] = entry.name();
    res[VfsQuery.Columns.URI] = entry.uri().toString();
    if (entry instanceof Vfs.Dir) {
      res[VfsQuery.Columns.TYPE] = Integer.valueOf(VfsQuery.Types.DIR);
    } else {
      res[VfsQuery.Columns.TYPE] = Integer.valueOf(VfsQuery.Types.FILE);
      final Vfs.File asFile = (Vfs.File) entry; 
      res[VfsQuery.Columns.SIZE] = Long.valueOf(asFile.size());
    }
    return res;
  }

  private static class CompareEntries implements Comparator<Vfs.Entry> {

    @Override
    public int compare(Vfs.Entry lh, Vfs.Entry rh) {
      final int byType = compareByType(lh, rh);
      return byType != 0 ? byType : compareByName(lh, rh);
    }
    
    private static int compareByType(Vfs.Entry lh, Vfs.Entry rh) {
      final int lhDir = lh instanceof Vfs.Dir ? 1 : 0;
      final int rhDir = rh instanceof Vfs.Dir ? 1 : 0;
      return rhDir - lhDir;
    }
    
    private static int compareByName(Vfs.Entry lh, Vfs.Entry rh) {
      return lh.name().compareToIgnoreCase(rh.name());
    }
  }
}
