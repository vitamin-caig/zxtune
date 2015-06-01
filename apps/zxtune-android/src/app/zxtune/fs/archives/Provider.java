/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.archives;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashSet;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import app.zxtune.Identifier;
import app.zxtune.TimeStamp;
import app.zxtune.Util;
import app.zxtune.ZXTune;
import app.zxtune.ZXTune.Module;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;

public final class Provider extends ContentProvider {
  
  private static final String TAG = Provider.class.getName();
  
  private Database db;
  
  @Override
  public boolean onCreate() {
    db = new Database(getContext());
    return true;
  }
  
  @Override
  public int delete(Uri arg0, String arg1, String[] arg2) {
    return 0;
  }

  @Override
  public String getType(Uri uri) {
    return Query.mimeTypeOf(uri);
  }

  @Override
  public Uri insert(Uri uri, ContentValues values) {
    if (!Query.isArchiveUri(uri)) {
      return null;
    }
    try {
      final Identifier fileId = new Identifier(Query.getPathFrom(uri));
      final Uri path = fileId.getDataLocation();
      Log.d(TAG, "Add archive content of " + path);
      final VfsFile file = (VfsFile)Vfs.resolve(path);
      final ByteBuffer data = file.getContent();
      final HashSet<Identifier> dirEntries = new HashSet<Identifier>();
      final Database.Transaction transaction = db.startTransaction();
      final AtomicInteger modulesCount = new AtomicInteger(0);
      try {
        ZXTune.detectModules(data, new ZXTune.ModuleDetectCallback() {
          @Override
          public void onModule(String subpath, Module module) {
            final Identifier moduleId = new Identifier(path, subpath);
            final DirEntry dirEntry = DirEntry.create(moduleId);
            
            final String author = module.getProperty(ZXTune.Module.Attributes.AUTHOR, "");
            final String title = module.getProperty(ZXTune.Module.Attributes.TITLE, "");
            final String description = Util.formatTrackTitle(author, title, "");
            final long frameDuration = module.getProperty(ZXTune.Properties.Sound.FRAMEDURATION, ZXTune.Properties.Sound.FRAMEDURATION_DEFAULT);
            final TimeStamp duration = TimeStamp.createFrom(frameDuration * module.getDuration(), TimeUnit.MICROSECONDS);
            final Track track = new Track(dirEntry.path.getFullLocation(), dirEntry.filename, description, duration);
            
            db.addTrack(track);
            final int doneTracks = modulesCount.incrementAndGet();
            if (0 == doneTracks % 100) {
              Log.d(TAG, "Found tracks: " + doneTracks);
            }
            module.release();
            if (!dirEntry.isRoot()) {
              db.addDirEntry(dirEntry);
              dirEntries.add(dirEntry.parent);
            }
          }
        });
        db.addArchive(new Archive(file.getUri(), modulesCount.get()));
        addDirEntries(dirEntries);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
      return Query.archiveUriFor(path);
    } catch (IOException e) {
    }
    return null;
  }
  
  private void addDirEntries(HashSet<Identifier> dirs) {
    final HashSet<Identifier> created = new HashSet<Identifier>();
    for (Identifier dir : dirs) {
      for (Identifier toCreate = dir; !created.contains(toCreate); ) {
        final DirEntry dirEntry = DirEntry.create(toCreate);
        if (dirEntry.isRoot()) {
          break;
        }
        Log.d(TAG, "Add dir " + dirEntry.path);
        db.addDirEntry(dirEntry);
        created.add(toCreate);
        toCreate = dirEntry.parent;
      }
    }
  }

  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }
  
  @Override
  public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
      String sortOrder) {
    final Uri path = Query.getPathFrom(uri);
    if (Query.isArchiveUri(uri)) {
      Log.d(TAG, "Query archive info of " + path);
      return db.queryArchive(path);
    } else if (Query.isListDirUri(uri)) {
      Log.d(TAG, "Query archive content of " + path);
      return db.queryListing(path);
    } else if (Query.isInfoUri(uri)) {
      Log.d(TAG, "Query archive item info of " + path);
      return db.queryInfo(path);
    } else {
      return null; 
    }
  }
}
