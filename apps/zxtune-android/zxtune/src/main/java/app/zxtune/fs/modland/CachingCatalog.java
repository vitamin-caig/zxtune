/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland;

import java.io.IOException;
import java.util.Locale;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.utils.ProgressCallback;
import app.zxtune.utils.StubProgressCallback;

final public class CachingCatalog implements Catalog {

  private final TimeStamp GROUPS_TTL = TimeStamp.fromDays(30);
  private final TimeStamp GROUP_TRACKS_TTL = TimeStamp.fromDays(14);

  private final Database db;
  private final Grouping authors;
  private final Grouping collections;
  private final Grouping formats;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db) {
    this.db = db;
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
      executor.executeQuery(category, new QueryCommand() {

        private final Timestamps.Lifetime lifetime = db.getGroupsLifetime(category, filter, GROUPS_TTL);

        @Override
        public boolean isCacheExpired() {
          return lifetime.isExpired();
        }

        @Override
        public void updateCache() throws IOException {
          db.runInTransaction(() -> {
            remote.queryGroups(filter, new GroupsVisitor() {
              // TODO: remove
              @Override
              public void setCountHint(int size) {}

              @Override
              public void accept(Group obj) {
                db.addGroup(category, obj);
              }
            }, progress);
            lifetime.update();
          });
        }

        @Override
        public boolean queryFromCache() {
          return db.queryGroups(category, filter, visitor);
        }
      });
    }

    @Override
    public Group getGroup(final int id) throws IOException {
      // It's impossible to fill all the cache, so query/update for specified group
      final String categoryElement = category.substring(0, category.length() - 1);
      return executor.executeFetchCommand(categoryElement, new FetchCommand<Group>() {

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
      executor.executeQuery("tracks", new QueryCommand() {

        private final Timestamps.Lifetime lifetime = db.getGroupTracksLifetime(category, id, GROUP_TRACKS_TTL);

        @Override
        public boolean isCacheExpired() {
          return lifetime.isExpired();
        }

        @Override
        public void updateCache() throws IOException {
          db.runInTransaction(() -> {
            remote.queryTracks(id, new TracksVisitor() {
              // TODO: remove
              @Override
              public void setCountHint(int size) {}

              @Override
              public boolean accept(Track obj) {
                db.addTrack(obj);
                db.addGroupTrack(category, id, obj);
                return true;
              }
            }, progress);
            lifetime.update();
          });
        }

        @Override
        public boolean queryFromCache() {
          return db.queryTracks(category, id, visitor);
        }
      });
    }

    @Override
    public Track getTrack(final int id, final String filename) throws IOException {
      // Just query all the category tracks and store found one
      final Track[] resultRef = {null};
      return executor.executeFetchCommand("track", new FetchCommand<Track>() {

        @Override
        public Track fetchFromCache() {
          return db.findTrack(category, id, filename);
        }

        @Override
        public Track updateCache() throws IOException {
          queryTracks(id, new TracksVisitor() {

            // TODO: remove
            @Override
            public void setCountHint(int size) {}

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
}
