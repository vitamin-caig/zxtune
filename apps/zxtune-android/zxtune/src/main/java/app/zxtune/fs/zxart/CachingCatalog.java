/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import java.io.IOException;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;

final public class CachingCatalog implements Catalog {

  private final TimeStamp AUTHORS_TTL = TimeStamp.fromDays(7);
  private final TimeStamp PARTIES_TTL = TimeStamp.fromDays(7);
  private final TimeStamp TRACKS_TTL = TimeStamp.fromDays(1);

  private final RemoteCatalog remote;
  private final Database db;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("zxart");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery("authors", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorsLifetime(AUTHORS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthors(new AuthorsVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

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
        return db.queryAuthors(visitor);
      }
    });
  }

  @Override
  public void queryAuthorTracks(final Author author, final TracksVisitor visitor)
      throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getAuthorTracksLifetime(author, TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryAuthorTracks(author, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

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
        return db.queryAuthorTracks(author, visitor);
      }
    });
  }

  @Override
  public void queryParties(final PartiesVisitor visitor) throws IOException {
    executor.executeQuery("parties", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getPartiesLifetime(PARTIES_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryParties(new PartiesVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

            @Override
            public void accept(Party obj) {
              db.addParty(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryParties(visitor);
      }
    });
  }

  @Override
  public void queryPartyTracks(final Party party, final TracksVisitor visitor)
      throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getPartyTracksLifetime(party, TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryPartyTracks(party, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

            @Override
            public void accept(Track obj) {
              db.addTrack(obj);
              db.addPartyTrack(party, obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryPartyTracks(party, visitor);
      }
    });
  }

  @Override
  public void queryTopTracks(final int limit, final TracksVisitor visitor) throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getTopLifetime(TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryTopTracks(limit, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int size) {}

            @Override
            public void accept(Track obj) {
              db.addTrack(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTopTracks(limit, visitor);
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
