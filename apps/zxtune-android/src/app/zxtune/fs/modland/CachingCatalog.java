/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.fs.VfsCache;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final static String CACHE_DIR_NAME = "ftp.modland.com";

  private final VfsCache cacheDir;
  private final Catalog remote;
  private final Grouping authors;
  private final Grouping collections;
  private final Grouping formats;
  private final Database db;

  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = VfsCache.create(context, CACHE_DIR_NAME); 
    this.remote = remote;
    this.authors = new CachedGrouping(Database.Tables.Authors.NAME, remote.getAuthors());
    this.collections = new CachedGrouping(Database.Tables.Collections.NAME, remote.getCollections());
    this.formats = new CachedGrouping(Database.Tables.Formats.NAME, remote.getFormats());
    this.db = db;
  }

  @Override
  public Grouping getAuthors() {
    return authors;
  }

  @Override
  public Grouping getCollections() {
    return collections;
  }
  
  @Override
  public Grouping getFormats() {
    return formats;
  }

  private class CachedGrouping implements Grouping {

    private final String category;
    private final Grouping remote;

    CachedGrouping(String category, Grouping remote) {
      this.category = category;
      this.remote = remote;
    }

    @Override
    public void query(String filter, GroupsVisitor visitor) throws IOException {
      final CountingGroupsVisitor count = new CountingGroupsVisitor(visitor);
      db.queryGroups(category, filter, count);
      if (0 == count.get()) {
        Log.d(TAG, category + " cache for filter '" + filter + "' is empty. Query from remote");
        final Database.Transaction transaction = db.startTransaction();
        try {
          remote.query(filter, new CachingGroupsVisitor(category, count));
          transaction.succeed();
          Log.d(TAG, "Cached " + count.get() + " " + category);
        } finally {
          transaction.finish();
        }
      }
    }

    @Override
    public Group query(int id) throws IOException {
      Group res = db.queryGroup(category, id);
      if (res == null) {
        Log.d(TAG, "No " + category + " id=" + id + " in cache. Query from remote");
        res = remote.query(id);
        if (res != null) {
          Log.d(TAG, "Cache " + category + " id=" + id);
          db.addGroup(category, res);
        }
      }
      return res;
    }

    @Override
    public void queryTracks(int id, TracksVisitor visitor) throws IOException {
      //TODO: make another way to check if cache is filled
      final CountingTracksVisitor count = new CountingTracksVisitor(visitor);
      db.queryTracks(category, id, count);
      if (0 == count.get()) {
        Log.d(TAG, "Tracks cache is empty for " + category + "=" + id);
        final Database.Transaction transaction = db.startTransaction();
        try {
          remote.queryTracks(id, new CachingTracksVisitor(category, id, count));
          transaction.succeed();
          Log.d(TAG, "Cached " + count.get() + " tracks");
        } finally {
          transaction.finish();
        }
      }
    }

    @Override
    public Track findTrack(int id, String filename) throws IOException {
      //TODO: keep all the paths decoded in DB
      final String encodedFilename = Uri.encode(filename).replace("!", "%21").replace("'", "%27").replace("(", "%28").replace(")", "%29");
      Track res = db.findTrack(category, id, encodedFilename);
      if (res == null) {
        Log.d(TAG, "Track " + filename + " not found in " + category + "=" + id);
        //fill cache
        queryTracks(id, new TracksVisitor() {
          @Override
          public void setCountHint(int size) {}

          @Override
          public boolean accept(Track obj) {
            return true;
          }
        });
        res = db.findTrack(category, id, encodedFilename);
      }
      return res;
    }
  }

  @Override
  public ByteBuffer getTrackContent(String id) throws IOException {
    final ByteBuffer cachedContent = cacheDir.getCachedFileContent(id);
    if (cachedContent != null) {
      return cachedContent;
    } else {
      final ByteBuffer content = remote.getTrackContent(id);
      cacheDir.putCachedFileContent(id, content);
      return content;
    }
  }

  private static class CountingGroupsVisitor implements GroupsVisitor {

    private final GroupsVisitor delegate;
    private int count;

    CountingGroupsVisitor(GroupsVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public void accept(Group obj) {
      delegate.accept(obj);
      ++count;
    }

    final int get() {
      return count;
    }
  }

  private class CachingGroupsVisitor implements GroupsVisitor {

    private final String category;
    private final GroupsVisitor delegate;

    CachingGroupsVisitor(String category, GroupsVisitor delegate) {
      this.category = category;
      this.delegate = delegate;
    }

    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public void accept(Group obj) {
      delegate.accept(obj);
      try {
        db.addGroup(category, obj);
      } catch (Exception e) {
        Log.d(TAG, "acceptAuthor()", e);
      }
    }
  }

  private static class CountingTracksVisitor implements TracksVisitor {

    private final TracksVisitor delegate;
    private int count;

    CountingTracksVisitor(TracksVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public boolean accept(Track obj) {
      final boolean result = delegate.accept(obj);
      ++count;
      return result;
    }

    final int get() {
      return count;
    }
  }

  private class CachingTracksVisitor implements TracksVisitor {

    private final String category;
    private final int group;
    private final TracksVisitor delegate;

    CachingTracksVisitor(String category, int group, TracksVisitor delegate) {
      this.category = category;
      this.group = group;
      this.delegate = delegate;
    }

    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public boolean accept(Track obj) {
      final boolean result = delegate.accept(obj);
      try {
        db.addTrack(obj);
        db.addGroupTrack(category, group, obj);
      } catch (Exception e) {
        Log.d(TAG, "addTrack()", e);
      }
      return result;
    }
  }
}
