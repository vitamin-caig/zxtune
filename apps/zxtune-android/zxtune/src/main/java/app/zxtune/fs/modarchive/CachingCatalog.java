/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modarchive;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FetchCommand;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = days(2);
  private final TimeStamp GENRES_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(2);
  
  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }
  
  private final Catalog remote;
  private final Database db;
  private final CommandExecutor executor;

  public CachingCatalog(Catalog remote, Database db) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor("modarchive");
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    executor.executeQueryCommand("authors", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired");
        remote.queryAuthors(new CachingAuthorsVisitor(visitor));
      }

      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(visitor);
      }
    });
  }

  @Override
  public void queryGenres(final GenresVisitor visitor) throws IOException {
    executor.executeQueryCommand("genres", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenresLifetime(GENRES_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Genres cache is empty/expired");
        remote.queryGenres(new CachingGenresVisitor(visitor));
      }

      @Override
      public boolean queryFromCache() {
        return db.queryGenres(visitor);
      }
    });
  }
  
  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
    executor.executeQueryCommand("tracks", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for author=%d", author.id);
        remote.queryTracks(author, new CachingTracksVisitor(visitor, author));
      }
    });
  }

  @Override
  public void queryTracks(final Genre genre, final TracksVisitor visitor) throws IOException {
    executor.executeQueryCommand("tracks", new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenreTracksLifetime(genre, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(genre, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for genre=%d", genre.id);
        remote.queryTracks(genre, new CachingTracksVisitor(visitor, genre));
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
      Log.d(TAG, "Use remote-side search");
      remote.findTracks(query, visitor);
    } else {
      Log.d(TAG, "Use local search");
      db.findTracks(query, visitor);
    }
  }
  
  @Override
  public ByteBuffer getTrackContent(final int id) throws IOException {
    return executor.executeFetchCommand("file", new FetchCommand<ByteBuffer>() {
      @Override
      public ByteBuffer fetchFromCache() {
        return db.getTrackContent(id);
      }

      @Override
      public ByteBuffer fetchFromRemote() throws IOException {
        final ByteBuffer res = remote.getTrackContent(id);
        db.addTrackContent(id, res);
        return res;
      }
    });
  }
  
  private class CachingAuthorsVisitor extends AuthorsVisitor {
    
    private final AuthorsVisitor delegate;
    
    CachingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }
    
    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      db.addAuthor(obj);
    }
  }

  private class CachingGenresVisitor extends GenresVisitor {
    
    private final GenresVisitor delegate;
    
    CachingGenresVisitor(GenresVisitor delegate) {
      this.delegate = delegate;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }
    
    @Override
    public void accept(Genre obj) {
      delegate.accept(obj);
      db.addGenre(obj);
    }
  }
  
  
  private class CachingTracksVisitor extends TracksVisitor {
    
    private final TracksVisitor delegate;
    private final Author author;
    private final Genre genre;
    
    public CachingTracksVisitor(TracksVisitor delegate, Author author) {
      this.delegate = delegate;
      this.author = author;
      this.genre = null;
    }
    
    public CachingTracksVisitor(TracksVisitor delegate, Genre genre) {
      this.delegate = delegate;
      this.author = null;
      this.genre = genre;
    }
    
    @Override
    public void setCountHint(int count) {
      delegate.setCountHint(count);
    }
    
    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      db.addTrack(obj);
      if (author != null) {
        db.addAuthorTrack(author, obj);
      }
      if (genre != null) {
        db.addGenreTrack(genre, obj);
      }
    }
  }
}
