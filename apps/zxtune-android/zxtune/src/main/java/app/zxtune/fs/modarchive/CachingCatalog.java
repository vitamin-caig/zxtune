/**
 * @file
 * @brief Caching catalog implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.*;
import app.zxtune.fs.http.HttpObject;

final class CachingCatalog extends Catalog {

  //private static final String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = days(2);
  private final TimeStamp GENRES_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(2);

  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }

  private final RemoteCatalog remote;
  private final Database db;
  private CacheDir cache;
  private final CommandExecutor executor;

  CachingCatalog(RemoteCatalog remote, Database db, CacheDir cache) {
    this.remote = remote;
    this.db = db;
    this.cache = cache.createNested("modarchive.org");
    this.executor = new CommandExecutor("modarchive");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "authors";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryAuthors(new AuthorsVisitor() {
          @Override
          public void accept(Author obj) {
            db.addAuthor(obj);
          }
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
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "genres";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenresLifetime(GENRES_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryGenres(new GenresVisitor() {
          @Override
          public void accept(Genre obj) {
            db.addGenre(obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryGenres(visitor);
      }
    });
  }

  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "tracks";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryTracks(author, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
            db.addAuthorTrack(author, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }
    });
  }

  @Override
  public void queryTracks(final Genre genre, final TracksVisitor visitor) throws IOException {
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "tracks";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenreTracksLifetime(genre, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        remote.queryTracks(genre, new TracksVisitor() {
          @Override
          public void accept(Track obj) {
            db.addTrack(obj);
            db.addGenreTrack(genre, obj);
          }
        });
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(genre, visitor);
      }
    });
  }

  @Override
  public boolean searchSupported() {
    return true;
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

  @Override
  @NonNull
  public ByteBuffer getTrackContent(final int id) throws IOException {
    return executor.executeDownloadCommand(new DownloadCommand() {
      @NonNull
      @Override
      public File getCache() throws IOException {
        return cache.findOrCreate(Integer.toString(id));
      }

      @NonNull
      @Override
      public HttpObject getRemote() throws IOException {
        return remote.getTrackObject(id);
      }
    });
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
