/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.amp.Database.CacheLifetime;

final class CachingCatalog extends Catalog {

  private final static String TAG = CachingCatalog.class.getName();

  private final static String CACHE_DIR_NAME = "amp.dascene.net";

  private final TimeStamp AUTHORS_TTL = days(7);
  private final TimeStamp COUNTRIES_TTL = days(7);
  private final TimeStamp TRACKS_TTL = days(7);
  
  private static TimeStamp days(int val) {
    return TimeStamp.createFrom(val, TimeUnit.DAYS);
  }
  
  private final VfsCache cacheDir;
  private final Catalog remote;
  private final Database db;

  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = VfsCache.create(context, CACHE_DIR_NAME); 
    this.remote = remote;
    this.db = db;
  }

  //TODO: extract to common place
  private interface QueryCommand {
    Database.CacheLifetime getLifetime();

    boolean queryFromCache();

    void queryFromRemote() throws IOException;
  }

  private void executeQuery(QueryCommand cmd) throws IOException {
    final Database.CacheLifetime lifetime = cmd.getLifetime();
    if (lifetime.isExpired() || !cmd.queryFromCache()) {
      IOException remoteError = null;
      final Database.Transaction transaction = db.startTransaction();
      try {
        cmd.queryFromRemote();
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!cmd.queryFromCache() && remoteError != null) {
        throw remoteError;
      }
    }
  }
  
  @Override
  public void queryAuthors(final String handleFilter, final AuthorsVisitor visitor) throws IOException {
    executeQuery(new QueryCommand() {
      
      @Override
      public CacheLifetime getLifetime() {
        return db.getLifetime("authors" + handleFilter, AUTHORS_TTL);
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for handleFilter=%s", handleFilter);
        remote.queryAuthors(handleFilter, new CachingAuthorsVisitor());
      }
      
      @Override
      public boolean queryFromCache() {
        db.queryAuthors(handleFilter, visitor);
        return true;
      }
    });
  }

  @Override
  public void queryAuthors(final Country country, final AuthorsVisitor visitor) throws IOException {
    executeQuery(new QueryCommand() {
      
      @Override
      public CacheLifetime getLifetime() {
        return db.getLifetime("country" + country.id, COUNTRIES_TTL);
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for country=%d", country.id);
        remote.queryAuthors(country, new CachingAuthorsVisitor(country));
      }
      
      @Override
      public boolean queryFromCache() {
        db.queryAuthors(country, visitor);
        return true;
      }
    });
  }
  
  @Override
  public void queryAuthors(final int id, final AuthorsVisitor visitor) throws IOException {
    executeQuery(new QueryCommand() {
      
      @Override
      public CacheLifetime getLifetime() {
        return db.getLifetime("author" + id, AUTHORS_TTL);
      }
      
      @Override
      public void queryFromRemote() throws IOException {
        Log.d(TAG, "Authors cache is empty/expired for id=%d", id);
        remote.queryAuthors(id, new CachingAuthorsVisitor());
      }
      
      @Override
      public boolean queryFromCache() {
        db.queryAuthors(id, visitor);
        return true;
      }
    });
  }

  @Override
  public void queryTracks(final Author author, final Integer id, final TracksVisitor visitor) throws IOException {
    executeQuery(new QueryCommand() {

      @Override
      public CacheLifetime getLifetime() {
        return id == null
            ? db.getLifetime("tracks" + author.id, TRACKS_TTL)
            : db.getStubLifetime();
      }

      @Override
      public boolean queryFromCache() {
        if (id != null) {
          db.queryTracks(id, visitor);
        } else {
          db.queryTracks(author, visitor);
        }
        return true;
      }

      @Override
      public void queryFromRemote() throws IOException {
        remote.queryTracks(author, id, new CachingTracksVisitor(author));
      }
    });
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
    
    private final Country country;
    
    CachingAuthorsVisitor() {
      this.country = null;
    }
    
    CachingAuthorsVisitor(Country country) {
      this.country = country;
    }

    @Override
    public void accept(Author obj) {
      try {
        db.addAuthor(obj);
        if (country != null) {
          db.addCountryAuthor(country, obj);
        }
      } catch (Exception e) {
        Log.d(TAG, e, "acceptAuthor()");
      }
    }
  }
  
  private class CachingTracksVisitor extends TracksVisitor {
    
    private final Author author;
    
    public CachingTracksVisitor(Author author) {
      this.author = author;
    }
    
    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj);
        db.addAuthorTrack(author, obj);
      } catch (Exception e) {
        Log.d(TAG, e, "acceptTrack()");
      }
    }
  }
}
