/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.archives;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.IOException;
import java.util.HashSet;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import app.zxtune.Core;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.Util;
import app.zxtune.ZXTune;
import app.zxtune.core.Module;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.core.Properties;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.dbhelpers.Transaction;

public final class Provider extends ContentProvider {

  private static final String TAG = Provider.class.getName();

  private Database db;

  @Override
  public boolean onCreate() {
    final Context ctx = getContext();
    if (ctx == null) {
      return false;
    }
    try {
      db = new Database(ctx);
      return true;
    } catch (IOException e) {
      return false;
    }
  }

  @Override
  public int delete(@NonNull Uri arg0, String arg1, String[] arg2) {
    return 0;
  }

  @Override
  public String getType(@NonNull Uri uri) {
    return Query.mimeTypeOf(uri);
  }

  @Override
  @Nullable
  public Uri insert(@NonNull Uri uri, ContentValues values) {
    if (!Query.isArchiveUri(uri)) {
      throw new IllegalArgumentException("Not an archive uri");
    }
    try {
      final Identifier fileId = new Identifier(Query.getPathFrom(uri));
      final Uri path = fileId.getDataLocation();
      Log.d(TAG, "Add archive content of %s", path);
      final VfsFile file = openFile(path);
      final HashSet<Identifier> dirEntries = new HashSet<>();
      final Transaction transaction = db.startTransaction();
      final AtomicInteger modulesCount = new AtomicInteger(0);
      final AtomicInteger logPeriod = new AtomicInteger(10);
      try {
        Core.detectModules(file, new ZXTune.ModuleDetectCallback() {
          @Override
          public void onModule(String subpath, Module module) {
            final Identifier moduleId = new Identifier(path, subpath);
            final DirEntry dirEntry = DirEntry.create(moduleId);

            try {
              final String author = module.getProperty(ModuleAttributes.AUTHOR, "");
              final String title = module.getProperty(ModuleAttributes.TITLE, "");
              final String description = Util.formatTrackTitle(author, title, "");
              final long frameDuration = module.getProperty(Properties.Sound.FRAMEDURATION, Properties.Sound.FRAMEDURATION_DEFAULT);
              final TimeStamp duration = TimeStamp.createFrom(frameDuration * module.getDuration(), TimeUnit.MICROSECONDS);
              final Track track = new Track(dirEntry.path.getFullLocation(), dirEntry.filename, description, duration);

              db.addTrack(track);
              final int doneTracks = modulesCount.incrementAndGet();
              final int period = logPeriod.get();
              if (0 == doneTracks % period) {
                Log.d(TAG, "Found tracks: %d", doneTracks);
                if (10 == doneTracks / period) {
                  logPeriod.set(period * 10);
                }
              }
            } catch (Exception e) {
              Log.w(TAG, e, "Skip module");
            } finally {
              module.release();
            }
            if (!dirEntry.isRoot()) {
              db.addDirEntry(dirEntry);
              dirEntries.add(dirEntry.parent);
            }
          }
        });
        Log.d(TAG, "Found %d tracks total", modulesCount.get());
        db.addArchive(new Archive(file.getUri(), modulesCount.get()));
        addDirEntries(dirEntries);
        transaction.succeed();
      } finally {
          transaction.finish();
      }
      return Query.archiveUriFor(path);
    } catch (Exception e) {
      Log.w(TAG, e, "InsertToArchive");
    }
    return null;
  }

  private static VfsFile openFile(Uri path) throws IOException {
    final VfsObject obj = Vfs.resolve(path);
    if (obj instanceof VfsFile) {
      return (VfsFile) obj;
    } else {
      throw new IOException("Failed to open " + path);
    }
  }

  private void addDirEntries(HashSet<Identifier> dirs) {
    final HashSet<Identifier> created = new HashSet<>();
    for (Identifier dir : dirs) {
      for (Identifier toCreate = dir; !created.contains(toCreate); ) {
        final DirEntry dirEntry = DirEntry.create(toCreate);
        if (dirEntry.isRoot()) {
          break;
        }
        Log.d(TAG, "Add dir %s", dirEntry.path);
        db.addDirEntry(dirEntry);
        created.add(toCreate);
        toCreate = dirEntry.parent;
      }
    }
  }

  @Override
  public int update(@NonNull Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs,
                      String sortOrder) {
    final Uri path = Query.getPathFrom(uri);
    if (Query.isArchiveUri(uri)) {
      return db.queryArchive(path);
    } else if (Query.isListDirUri(uri)) {
      return db.queryListing(path);
    } else if (Query.isInfoUri(uri)) {
      return db.queryInfo(path);
    } else {
      throw new IllegalArgumentException("Not an archive uri");
    }
  }
}
