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
import java.util.concurrent.TimeUnit;

import android.content.Context;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final static String CACHE_DIR_NAME = "ftp.modland.com";

  private final TimeStamp GROUPS_TTL = days(30);
  private final TimeStamp GROUP_TRACKS_TTL = days(14);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }
  
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
    public void query(final String filter, final GroupsVisitor visitor) throws IOException {
      Utils.executeQueryCommand(new QueryCommand() {
        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getGroupsLifetime(category, filter, GROUPS_TTL);
        }
        
        @Override
        public Transaction startTransaction() {
          return db.startTransaction();
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroups(category, filter, visitor);
        }

        @Override
        public void queryFromRemote() throws IOException {
          Log.d(TAG, "%s cache is empty/expired for filter=%s", category, filter);
          remote.query(filter, new CachingGroupsVisitor(category));
        }
      });
    }

    @Override
    public Group query(int id) throws IOException {
      Group res = db.queryGroup(category, id);
      if (res == null) {
        Log.d(TAG, "No %s id=%d in cache. Query from remote", category, id);
        res = remote.query(id);
        if (res != null) {
          Log.d(TAG, "Cache %s id=%d", category, id);
          db.addGroup(category, res);
        }
      }
      return res;
    }

    @Override
    public void queryTracks(final int id, final TracksVisitor visitor) throws IOException {
      Utils.executeQueryCommand(new QueryCommand() {
        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getGroupTracksLifetime(category, id, GROUP_TRACKS_TTL);
        }
        
        @Override
        public Transaction startTransaction() {
          return db.startTransaction();
        }

        @Override
        public boolean queryFromCache() {
          return db.queryTracks(category, id, visitor);
        }

        @Override
        public void queryFromRemote() throws IOException {
          Log.d(TAG, "Tracks cache is empty/expired for %s=%d", category, id);
          remote.queryTracks(id, new CachingTracksVisitor(category, id));
        }
      });
    }

    @Override
    public Track findTrack(int id, String filename) throws IOException {
      Track res = db.findTrack(category, id, filename);
      if (res == null) {
        Log.d(TAG, "Track %s not found in %s=%d", filename, category, id);
        //fill cache
        queryTracks(id, new TracksVisitor() {

          @Override
          public boolean accept(Track obj) {
            return true;
          }
        });
        res = db.findTrack(category, id, filename);
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

  private class CachingGroupsVisitor extends GroupsVisitor {

    private final String category;

    CachingGroupsVisitor(String category) {
      this.category = category;
    }

    @Override
    public void accept(Group obj) {
      try {
        db.addGroup(category, obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptGroup()");
      }
    }
  }

  private class CachingTracksVisitor extends TracksVisitor {

    private final String category;
    private final int group;

    CachingTracksVisitor(String category, int group) {
      this.category = category;
      this.group = group;
    }

    @Override
    public boolean accept(Track obj) {
      try {
        db.addTrack(obj);
        db.addGroupTrack(category, group, obj);
      } catch (Exception e) {
        Log.d(TAG, e, "addTrack()");
      }
      return true;
    }
  }
}
