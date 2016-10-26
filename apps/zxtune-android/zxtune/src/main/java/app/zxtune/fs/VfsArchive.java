/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir.Visitor;
import app.zxtune.fs.archives.Archive;
import app.zxtune.fs.archives.DirEntry;
import app.zxtune.fs.archives.Entry;
import app.zxtune.fs.archives.Query;
import app.zxtune.fs.archives.Track;

public final class VfsArchive {
  
  private final static String TAG = VfsArchive.class.getName();
  
  /*
   * @return
   * 
   * VfsDir - browsable archive
   * VfsFile - single file to play (or no files to play)
   * null - unknown status
   */
  public static VfsObject browseCached(VfsFile file) {
    if (file instanceof ArchiveFile) {
      return file;
    }
    final Context appContext = MainApplication.getInstance();
    final ContentResolver resolver = appContext.getContentResolver();
    return browseCached(resolver, file);
  }
  
  private static VfsObject browseCached(ContentResolver resolver, VfsFile file) {
    final Uri uri = file.getUri();
    final Integer modulesInArchive = getModulesInArchive(resolver, uri);
    if (modulesInArchive == null) {
      Log.d(TAG, "Unknown archive %s", uri);
      return null;
    } else if (modulesInArchive < 2) {
      Log.d(TAG, "Too few modules in archive %s", uri);
      return file;
    }
    return new ArchiveRoot(resolver, file);
  }
  
  private static Integer getModulesInArchive(ContentResolver resolver, Uri uri) {
    final Cursor cursor = resolver.query(Query.archiveUriFor(uri), null, null, null, null);
    try {
      if (cursor.moveToFirst()) {
        final Archive arch = Archive.fromCursor(cursor);
        return arch.modules;
      }
    } finally {
      cursor.close();
    }
    return null;
  }
  
  /*
   * @return
   * 
   * VfsDir - browsable archive
   * VfsFile - single file to play
   * null - nothing to play
   */
  public static VfsObject browse(VfsFile file) {
    final Uri uri = file.getUri();
    final Context appContext = MainApplication.getInstance();
    final ContentResolver resolver = appContext.getContentResolver();
    final Uri newUri = resolver.insert(Query.archiveUriFor(uri), new ContentValues());
    if (newUri == null) {
      return null;
    } else {
      return browseCached(file);
    }
  }
  
  public static VfsObject resolve(Uri uri) throws IOException {
    final Context appContext = MainApplication.getInstance();
    final ContentResolver resolver = appContext.getContentResolver();
    return resolve(resolver, uri);
  }
  
  private static VfsObject resolve(ContentResolver resolver, Uri uri) throws IOException {
    final Identifier id = new Identifier(uri);
    final String subpath = id.getSubpath();
    if (TextUtils.isEmpty(subpath)) {
      return resolveFile(resolver, uri);
    } else {
      return resolveArchive(resolver, uri);
    }
  }
  
  private static VfsObject resolveFile(ContentResolver resolver, Uri uri) throws IOException {
    final VfsObject obj = Vfs.resolve(uri);
    if (obj instanceof VfsFile) {
      final VfsObject cached = browseCached(resolver, (VfsFile) obj);
      if (cached != null) {
        return cached;
      }
    }
    return obj;
  }
  
  private static VfsObject resolveArchive(ContentResolver resolver, Uri uri) throws IOException {
    final Cursor cursor = resolver.query(Query.infoUriFor(uri), null, null, null, null);
    try {
      if (cursor.moveToFirst()) {
        final Entry entry = Entry.fromCursor(cursor);
        if (entry.track != null) {
          return new ArchiveFile(entry.track);
        } else {
          final VfsObject parent = resolve(resolver, entry.dirEntry.parent.getFullLocation());
          return new ArchiveDir(resolver, parent, entry.dirEntry);
        }
      }
    } finally {
      cursor.close();
    }
    return null;
  }
  
  private static void listArchive(ContentResolver resolver, VfsObject parent, Visitor visitor) {
    final Cursor cursor = resolver.query(Query.listDirUriFor(parent.getUri()), null, null, null, null);
    try {
      visitor.onItemsCount(cursor.getCount());
      while (cursor.moveToNext()) {
        final Entry entry = Entry.fromCursor(cursor);
        if (entry.track != null) {
          visitor.onFile(new ArchiveFile(entry.track));
        } else {
          visitor.onDir(new ArchiveDir(resolver, parent, entry.dirEntry));
        }
      }
    } finally {
      cursor.close();
    }
  }
  
  private static class ArchiveRoot extends StubObject implements VfsDir {

    private final ContentResolver resolver;
    private final VfsFile file;
    
    ArchiveRoot(ContentResolver resolver, VfsFile file) {
      this.resolver = resolver;
      this.file = file;
    }
    
    @Override
    public Uri getUri() {
      return file.getUri();
    }
    
    @Override
    public String getName() {
      return file.getName();
    }
    
    @Override
    public VfsObject getParent() {
      return file.getParent();
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      listArchive(resolver, this, visitor);
    }
  }
  
  private static class ArchiveDir extends StubObject implements VfsDir {

    private final ContentResolver resolver;
    private final VfsObject parent;
    private final DirEntry dir;
    
    ArchiveDir(ContentResolver resolver, VfsObject parent, DirEntry dir) {
      this.resolver = resolver;
      this.parent = parent;
      this.dir = dir;
    }

    @Override
    public Uri getUri() {
      return dir.path.getFullLocation();
    }

    @Override
    public String getName() {
      return dir.filename;
    }

    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      listArchive(resolver, this, visitor);
    }
  }
  
  private static class ArchiveFile extends StubObject implements VfsFile {
    
    private final Track track;
    
    ArchiveFile(Track track) {
      this.track = track;
    }
    
    @Override
    public Uri getUri() {
      return track.path;
    }
    
    @Override
    public String getName() {
      return track.filename;
    }

    @Override
    public String getDescription() {
      return track.description;
    }
    
    @Override
    public VfsObject getParent() {
      return null;
    }
    
    @Override
    public String getSize() {
      return track.duration.toString();
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return null;
    }
  }

  public static boolean checkIfArchive(VfsDir dir) {
    return dir instanceof ArchiveRoot || dir instanceof ArchiveDir;
  }
}
