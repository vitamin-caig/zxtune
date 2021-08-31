/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import java.io.IOException;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.utils.ProgressCallback;

final public class CachingCatalog implements Catalog {

  private final TimeStamp AUTHORS_TTL = TimeStamp.fromDays(2);
  private final TimeStamp GENRES_TTL = TimeStamp.fromDays(30);
  private final TimeStamp TRACKS_TTL = TimeStamp.fromDays(2);

  private final RemoteCatalog remote;
  private final Database db;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("modarchive");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor, final ProgressCallback progress) throws IOException {
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
            public void setCountHint(int count) {}

            @Override
            public void accept(Author obj) {
              db.addAuthor(obj);
            }
          }, progress);
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
  public void queryGenres(final GenresVisitor visitor) throws IOException {
    executor.executeQuery("genres", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getGenresLifetime(GENRES_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryGenres(new GenresVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Genre obj) {
              db.addGenre(obj);
            }
          });
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryGenres(visitor);
      }
    });
  }

  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor,
                          final ProgressCallback progress) throws IOException {
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
          }, progress);
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
  public void queryTracks(final Genre genre, final TracksVisitor visitor,
                          final ProgressCallback progress) throws IOException {
    executor.executeQuery("tracks", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getGenreTracksLifetime(genre, TRACKS_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
      }

      @Override
      public void updateCache() throws IOException {
        db.runInTransaction(() -> {
          remote.queryTracks(genre, new TracksVisitor() {
            // TODO: remove
            @Override
            public void setCountHint(int count) {}

            @Override
            public void accept(Track obj) {
              db.addTrack(obj);
              db.addGenreTrack(genre, obj);
            }
          }, progress);
          lifetime.update();
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(genre, visitor);
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

  @Override
  public void findRandomTracks(TracksVisitor visitor) throws IOException {
    //TODO: another method?
    if (remote.searchSupported()) {
      remote.findRandomTracks(new TracksCacher(visitor));
    } else {
      db.queryRandomTracks(visitor);
    }
  }

  private class TracksCacher implements TracksVisitor {

    private final TracksVisitor delegate;

    TracksCacher(TracksVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void accept(Track obj) {
      db.addTrack(obj);
      delegate.accept(obj);
    }

    // TODO: remove
    @Override
    public void setCountHint(int count) {}
  }
}
