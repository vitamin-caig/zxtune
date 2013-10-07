/**
 * @file
 * @brief Implementation of Cursor based on Vfs content
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import android.database.AbstractCursor;

class VfsCursor extends AbstractCursor {
  
  private ArrayList<VfsDir> dirs;
  private ArrayList<VfsFile> files;
  private Object[][] resolvedEntries;
  private int resolvedDirsCount;
  private int resolvedFilesCount;

  public VfsCursor(VfsDir dir) throws IOException {
    dirs = new ArrayList<VfsDir>();
    files = new ArrayList<VfsFile>();
    dir.enumerate(new VfsDir.Visitor() {

      @Override
      public Status onFile(VfsFile file) {
        files.add(file);
        return Status.CONTINUE;
      }
      
      @Override
      public Status onDir(VfsDir dir) {
        dirs.add(dir);
        return Status.CONTINUE;
      }
    });
    final Comparator<VfsObject> comparator = new Comparator<VfsObject>() {
      @Override
      public int compare(VfsObject lh, VfsObject rh) {
        return lh.getName().compareToIgnoreCase(rh.getName());
      }
    };

    Collections.sort(dirs, comparator);
    Collections.sort(files, comparator);
  }
  
  @Override
  public void close() {
    dirs = null;
    files = null;
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
    return (dirs != null ? dirs.size() : resolvedDirsCount)
        + (files != null ? files.size() : resolvedFilesCount);
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
      resolvedEntries = new Object[dirs.size() + files.size()][];
    }
    if (null == resolvedEntries[mPos]) {
      final int dirsCount = dirs != null ? dirs.size() : resolvedDirsCount;
      if (mPos < dirsCount) {
        resolvedEntries[mPos] = createDirEntry(mPos, dirs.get(mPos));
        if (dirsCount == ++resolvedDirsCount) {
          dirs = null;
        } else {
          dirs.set(mPos, null);
        }
      } else {
        final int fileIndex = mPos - dirsCount;
        resolvedEntries[mPos] = createFileEntry(mPos, files.get(fileIndex));
        if (files.size() == ++resolvedFilesCount) {
          files = null;
        } else {
          files.set(fileIndex, null);
        }
      }
    }
    return resolvedEntries[mPos];
  }

  private static Object[] createDirEntry(int id, VfsDir dir) {
    final Object[] res = createObjectEntry(id, dir); 
    res[VfsQuery.Columns.TYPE] = Integer.valueOf(VfsQuery.Types.DIR);
    return res;
  }
  
  private static Object[] createFileEntry(int id, VfsFile file) {
    final Object[] res = createObjectEntry(id, file);
    res[VfsQuery.Columns.TYPE] = Integer.valueOf(VfsQuery.Types.FILE);
    res[VfsQuery.Columns.SIZE] = file.getSize();
    return res;
  }

  private static Object[] createObjectEntry(int id, VfsObject obj) {
    final Object[] res = new Object[VfsQuery.Columns.TOTAL];
    res[VfsQuery.Columns.ID] = Long.valueOf(id);
    res[VfsQuery.Columns.NAME] = obj.getName();
    res[VfsQuery.Columns.URI] = obj.getUri().toString();
    return res;
  }
}
