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
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp GENRES_TTL = days(30);
  private final TimeStamp TRACKS_TTL = days(7);
  
  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }
  
  private final Catalog remote;
  private final Database db;
  private final VfsCache cacheDir;

  public CachingCatalog(Catalog remote, Database db, VfsCache cacheDir) {
    this.remote = remote;
    this.db = db;
    this.cacheDir = cacheDir; 
  }

  @Override
  public void queryAuthors(final AuthorsVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorsLifetime(AUTHORS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired");
        remote.queryAuthors(new CachingAuthorsVisitor());
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryAuthors(visitor);
      }
    });
  }

  @Override
  public void queryGenres(final GenresVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {
      
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenresLifetime(GENRES_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Genres cache is empty/expired");
        remote.queryGenres(new CachingGenresVisitor());
      }
      
      @Override
      public boolean queryFromCache() {
        return db.queryGenres(visitor);
      }
    });
  }
  
  @Override
  public void queryTracks(final Author author, final TracksVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getAuthorTracksLifetime(author, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(author, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for author=%d", author.id);
        remote.queryTracks(author, new CachingTracksVisitor(author));
      }
    });
  }

  @Override
  public void queryTracks(final Genre genre, final TracksVisitor visitor) throws IOException {
    Utils.executeQueryCommand(new QueryCommand() {

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getGenreTracksLifetime(genre, TRACKS_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public boolean queryFromCache() {
        return db.queryTracks(genre, visitor);
      }

      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Tracks cache is empty/expired for genre=%d", genre.id);
        remote.queryTracks(genre, new CachingTracksVisitor(genre));
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
  public ByteBuffer getTrackContent(int id) throws IOException {
    final String filename = Integer.toString(id);
    final ByteBuffer cachedContent = cacheDir.getCachedFileContent(filename);
    if (cachedContent != null) {
      return cachedContent;
    } else {
      final ByteBuffer content = remote.getTrackContent(id);
      cacheDir.putCachedFileContent(filename, content);
      return content;
    }
  }
  
  private class CachingAuthorsVisitor extends AuthorsVisitor {
    
    @Override
    public void accept(Author obj) {
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptAuthor()");
      }
    }
  }

  private class CachingGenresVisitor extends GenresVisitor {
    
    @Override
    public void accept(Genre obj) {
      try {
        db.addGenre(obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptGenre()");
      }
    }
  }
  
  
  private class CachingTracksVisitor extends TracksVisitor {
    
    private final Author author;
    private final Genre genre;
    
    public CachingTracksVisitor(Author author) {
      this.author = author;
      this.genre = null;
    }
    
    public CachingTracksVisitor(Genre genre) {
      this.author = null;
      this.genre = genre;
    }
    
    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj);
        if (author != null) {
          db.addAuthorTrack(author, obj);
        }
        if (genre != null) {
          db.addGenreTrack(genre, obj);
        }
      } catch (Exception e) {
        Log.d(TAG, e, "acceptTrack()");
      }
    }
  }
}
