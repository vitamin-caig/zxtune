/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import java.io.IOException;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;

final public class CachingCatalog implements Catalog {

  private final TimeStamp GROUPS_TTL = TimeStamp.fromDays(30);
  private final TimeStamp AUTHORS_TTL = TimeStamp.fromDays(30);
  private final TimeStamp TRACKS_TTL = TimeStamp.fromDays(30);

  private final RemoteCatalog remote;
  private final Database db;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("amp");
  }

  @Override
  public void queryGroups(final GroupsVisitor visitor) throws IOException {
    executor.executeQuery("groups", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getGroupsLifetime(GROUPS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryGroups(new GroupsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Group obj) {
              db.addGroup(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryGroups(visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final String handleFilter, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorsLifetime(handleFilter, AUTHORS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthors(handleFilter, new AuthorsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Author obj) {
              db.addAuthor(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(handleFilter, visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final Country country, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getCountryLifetime(country, AUTHORS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthors(country, new AuthorsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Author obj) {
              db.addAuthor(obj);
              db.addCountryAuthor(country, obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(country, visitor);
      }
    });
  }

  @Override
  public void queryAuthors(final Group group, final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getGroupLifetime(group, AUTHORS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthors(group, new AuthorsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Author obj) {
              db.addAuthor(obj);
              db.addGroupAuthor(group, obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(group, visitor);
      }
    });
  }

  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryTracks(author, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Track obj) {
              db.addTrack(obj);
              db.addAuthorTrack(author, obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }
    });
  }

  @Override
  public void findTracks(String query, FoundTracksVisitor visitor) throws IOException {
    if (remote.searchSupported()) {
      remote.findTracks(query, visitor);
    } else {
      db.findTracks(query, visitor);
    }
  }
}
