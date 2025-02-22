package app.zxtune.fs.archives;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.Util;
import app.zxtune.core.Core;
import app.zxtune.core.Identifier;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.fs.VfsFile;
import app.zxtune.utils.ProgressCallback;

public class ArchivesService {

  private static final String TAG = ArchivesService.class.getName();

  @FunctionalInterface
  public interface ListingCallback {
    void onEntry(Entry entry);
  }

  private final Database db;

  public ArchivesService(Context ctx) {
    this.db = new Database(ctx);
  }

  @Nullable
  public final Archive findArchive(Uri uri) {
    final Cursor cursor = db.queryArchive(uri);
    try {
      if (cursor.moveToFirst()) {
        return Archive.fromCursor(cursor);
      }
    } finally {
      cursor.close();
    }
    return null;
  }

  // Order and size match is not guaranteed!!!!
  public final ArrayList<Archive> findArchives(List<Uri> uris) {
    final ArrayList<Archive> result = new ArrayList<>(uris.size());
    if (!uris.isEmpty()) {
      final Cursor cursor = db.queryArchives(uris);
      try {
        while (cursor.moveToNext()) {
          result.add(Archive.fromCursor(cursor));
        }
      } finally {
        cursor.close();
      }
    }
    return result;
  }

  @Nullable
  public final Entry resolve(Uri uri) {
    final Cursor cursor = db.queryInfo(uri);
    try {
      if (cursor.moveToFirst()) {
        return Entry.fromCursor(cursor);
      }
    } finally {
      cursor.close();
    }
    return null;
  }

  public final void listDir(Uri uri, ListingCallback cb) {
    final Cursor cursor = db.queryListing(uri);
    try {
      while (cursor.moveToNext()) {
        cb.onEntry(Entry.fromCursor(cursor));
      }
    } finally {
      cursor.close();
    }
  }

  public final Archive analyzeArchive(VfsFile file, final ProgressCallback cb) throws IOException {
    final Uri path = file.getUri();
    Log.d(TAG, "Add archive content of %s", path);
    final Archive[] result = {null};
    final HashSet<Identifier> dirEntries = new HashSet<>();
    final int[] report = new int[]{0, 10};
    db.runInTransaction(() -> {
      Core.detectModules(file, (moduleId, module) -> {
        final DirEntry dirEntry = DirEntry.create(moduleId);

        try {
          final String author = module.getProperty(ModuleAttributes.AUTHOR, "");
          final String title = module.getProperty(ModuleAttributes.TITLE, "");
          final String description = Util.formatTrackTitle(title, author, "");
          final TimeStamp duration = module.getDuration();
          final Track track = new Track(dirEntry.path.getFullLocation(), dirEntry.filename, description, duration);
          module.release();

          db.addTrack(track);
          ++report[0];
          final int doneTracks = report[0];
          final int period = report[1];
          if (0 == doneTracks % period) {
            Log.d(TAG, "Found tracks: %d", doneTracks);
            if (10 == doneTracks / period) {
              report[1] *= 10;
            }
          }
        } catch (Exception e) {
          Log.w(TAG, e, "Skip module");
        }
        if (!dirEntry.isRoot()) {
          db.addDirEntry(dirEntry);
          dirEntries.add(dirEntry.parent);
        }
      }, cb);
      result[0] = new Archive(path, report[0]);
      Log.d(TAG, "Found %d tracks total", result[0].modules);
      db.addArchive(result[0]);
      addDirEntries(dirEntries);
    });
    return result[0];
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
}
