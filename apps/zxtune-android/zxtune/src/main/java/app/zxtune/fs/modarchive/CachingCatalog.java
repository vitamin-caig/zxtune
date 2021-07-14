/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.utils.ProgressCallback;

final public class CachingCatalog extends Catalog {

  private final TimeStamp AUTHORS_TTL = days(2);
  private final TimeStamp GENRES_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(2);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

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

  private class TracksCacher extends TracksVisitor {

    private final TracksVisitor delegate;

    TracksCacher(TracksVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void accept(Track obj) {
      db.addTrack(obj);
      delegate.accept(obj);
    }
  }
}
