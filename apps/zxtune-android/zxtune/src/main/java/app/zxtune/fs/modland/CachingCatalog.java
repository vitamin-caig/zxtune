/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import app.zxtune.StubProgressCallback;
import app.zxtune.TimeStamp;
import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.DownloadCommand;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.http.HttpObject;

final public class CachingCatalog extends Catalog {

  private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp GROUPS_TTL = days(30);
  private final TimeStamp GROUP_TRACKS_TTL = days(14);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final RemoteCatalog remote;
  private final Database db;
  private final CacheDir cache;
  private final Grouping authors;
  private final Grouping collections;
  private final Grouping formats;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db, CacheDir cache) {
    this.remote = remote;
    this.db = db;
    this.cache = cache.createNested("ftp.modland.com");
    this.authors = new CachedGrouping(Database.Tables.Authors.NAME, remote.getAuthors());
    this.collections = new CachedGrouping(Database.Tables.Collections.NAME, remote.getCollections());
    this.formats = new CachedGrouping(Database.Tables.Formats.NAME, remote.getFormats());
    this.executor = new CommandExecutor("modland");
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
    public void queryGroups(final String filter, final GroupsVisitor visitor,
                            final ProgressCallback progress) throws IOException {
      executor.executeQuery(new QueryCommand() {

        @Override
        public String getScope() {
          return category;
        }

        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getGroupsLifetime(category, filter, GROUPS_TTL);
        }

        @Override
        public Transaction startTransaction() throws IOException {
          return db.startTransaction();
        }

        @Override
        public void updateCache() throws IOException {
          remote.queryGroups(filter, new GroupsVisitor() {
            @Override
            public void accept(Group obj) {
              db.addGroup(category, obj);
            }
          }, progress);
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroups(category, filter, visitor);
        }
      });
    }

    @Override
    @NonNull
    public Group getGroup(final int id) throws IOException {
      // It's impossible to fill all the cache, so query/update for specified group
      final String categoryElement = category.substring(0, category.length() - 1);
      return executor.executeFetchCommand(new FetchCommand<Group>() {

        @Override
        public String getScope() {
          return categoryElement;
        }

        @Override
        public Group fetchFromCache() {
          return db.queryGroup(category, id);
        }

        @Override
        public Group updateCache() throws IOException {
          final Group res = remote.getGroup(id);
          db.addGroup(category, res);
          return res;
        }
      });
    }

    @Override
    public void queryTracks(final int id, final TracksVisitor visitor,
                            final ProgressCallback progress) throws IOException {
      executor.executeQuery(new QueryCommand() {

        @Override
        public String getScope() {
          return "tracks";
        }

        @Override
        public Timestamps.Lifetime getLifetime() {
          return db.getGroupTracksLifetime(category, id, GROUP_TRACKS_TTL);
        }

        @Override
        public Transaction startTransaction() throws IOException {
          return db.startTransaction();
        }

        @Override
        public void updateCache() throws IOException {
          remote.queryTracks(id, new TracksVisitor() {
            @Override
            public boolean accept(Track obj) {
              db.addTrack(obj);
              db.addGroupTrack(category, id, obj);
              return true;
            }
          }, progress);
        }

        @Override
        public boolean queryFromCache() {
          return db.queryTracks(category, id, visitor);
        }
      });
    }

    @Override
    @NonNull
    public Track getTrack(final int id, final String filename) throws IOException {
      // Just query all the category tracks and store found one
      final Track[] resultRef = {null};
      return executor.executeFetchCommand(new FetchCommand<Track>() {
        @Override
        public String getScope() {
          return "track";
        }

        @Override
        public Track fetchFromCache() {
          return db.findTrack(category, id, filename);
        }

        @Override
        public Track updateCache() throws IOException {
          queryTracks(id, new TracksVisitor() {

            @Override
            public boolean accept(Track obj) {
              if (obj.filename.equals(filename)) {
                resultRef[0] = obj;
              }
              return true;
            }
          }, StubProgressCallback.instance());
          final Track result = resultRef[0];
          if (result != null) {
            return result;
          }
          throw new IOException(String.format(Locale.US, "Failed to get track '%s' with id=%d", filename, id));
        }
      });
    }
  }

  @Override
  @NonNull
  public ByteBuffer getTrackContent(final String id) throws IOException {
    return executor.executeDownloadCommand(new DownloadCommand() {
      @NonNull
      @Override
      public File getCache() throws IOException {
        return cache.findOrCreate(id);
      }

      @NonNull
      @Override
      public HttpObject getRemote() throws IOException {
        return remote.getTrackObject(id);
      }
    });
  }

  @Nullable
  public final File getTrackCache(String id) {
    return cache.find(id);
  }
}
