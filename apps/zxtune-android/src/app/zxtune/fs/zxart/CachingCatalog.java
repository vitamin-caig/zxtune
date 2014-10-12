/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.zxart;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.util.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.VfsCache;

final class CachingCatalog extends Catalog {
  
  private final static String TAG = CachingCatalog.class.getName(); 
  private final static String CACHE_DIR_NAME = "www.zxart.ee";
  
  private final TimeStamp AUTHORS_TTL = days(7); 
  private final TimeStamp PARTIES_TTL = days(60);
  private final TimeStamp TOP_TTL = days(1);
  
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

  @Override
  public void queryAuthors(AuthorsVisitor visitor, Integer id) throws IOException {
    final Database.CacheLifetime lifetime = db.getAuthorsLifetime(null);
    if (lifetime.isExpired(AUTHORS_TTL)
        || !db.queryAuthors(visitor, id)) {
      IOException remoteError = null;
      Log.d(TAG, "Authors cache is empty/expired for id=" + id);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthors(new CachingAuthorsVisitor(), null);
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!db.queryAuthors(visitor, id) && remoteError != null) {
        throw remoteError;
      }
    }
  }

  @Override
  public void queryAuthorTracks(TracksVisitor visitor, Integer id, Integer author)
      throws IOException {
    final Database.CacheLifetime lifetime = db.getAuthorsLifetime(author);
    if (lifetime.isExpired(AUTHORS_TTL)
        || !db.queryAuthorTracks(visitor, id, author)) {
      IOException remoteError = null;
      Log.d(TAG, "Tracks cache is empty/expired for id=" + id + " author=" + author);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthorTracks(new CachingTracksVisitor(author, null), null, author);
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!db.queryAuthorTracks(visitor, id, author) && remoteError != null) {
        throw remoteError;
      }
    }
  }

  @Override
  public void queryParties(PartiesVisitor visitor, Integer id) throws IOException {
    final Database.CacheLifetime lifetime = db.getPartiesLifetime(null);
    if (lifetime.isExpired(PARTIES_TTL)
        || !db.queryParties(visitor, id)) {
      IOException remoteError = null;
      Log.d(TAG, "Parties cache is empty/expired for id=" + id);
      final Database.Transaction transaction = db.startTransaction();
      try {
        //query all
        remote.queryParties(new CachingPartiesVisitor(), null);
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!db.queryParties(visitor, id) && remoteError != null) {
        throw remoteError;
      }
    }
  }

  @Override
  public void queryPartyTracks(TracksVisitor visitor, Integer id, Integer party) throws IOException {
    final Database.CacheLifetime lifetime = db.getPartiesLifetime(party);
    if (lifetime.isExpired(PARTIES_TTL)
        || !db.queryPartyTracks(visitor, id, party)) {
      IOException remoteError = null;
      Log.d(TAG, "Tracks cache is empty/expired for id=" + id + " party=" + party);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryPartyTracks(new CachingTracksVisitor(null, party), null, party);
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!db.queryPartyTracks(visitor, id, party) && remoteError != null) {
        throw remoteError;
      }
    }
  }

  @Override
  public void queryTopTracks(TracksVisitor visitor, Integer id, int limit) throws IOException {
    final Database.CacheLifetime lifetime = db.getTopLifetime();
    if (lifetime.isExpired(TOP_TTL)
        || !db.queryTopTracks(visitor, id, limit)) {
      IOException remoteError = null;
      Log.d(TAG, "Top tracks cache is empty/expired");
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryTopTracks(new CachingTracksVisitor(null, null), id, limit);
        lifetime.update();
        transaction.succeed();
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
      if (!db.queryTopTracks(visitor, id, limit) && remoteError != null) {
        throw remoteError;
      }
    }
  }
  
  @Override
  public ByteBuffer getTrackContent(int id) throws IOException {
    final String strId = Integer.toString(id);
    final ByteBuffer cachedContent = cacheDir.getCachedFileContent(strId);
    if (cachedContent != null) {
      return cachedContent;
    } else {
      final ByteBuffer content = remote.getTrackContent(id);
      cacheDir.putCachedFileContent(strId, content);
      return content;
    }
  }

  private class CachingAuthorsVisitor implements AuthorsVisitor {
    
    @Override
    public void setCountHint(int hint) {
    }

    @Override
    public void accept(Author obj) {
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, "acceptAuthor()", e);
      }
    }
  }

  private class CachingPartiesVisitor implements PartiesVisitor {
    
    @Override
    public void setCountHint(int hint) {
    }

    @Override
    public void accept(Party obj) {
      try {
        db.addParty(obj);
      } catch (Exception e) {
        Log.d(TAG, "acceptParty()", e);
      }
    }
  }
  
  private class CachingTracksVisitor implements TracksVisitor {

    private final Integer author;
    private final Integer party;
    
    private CachingTracksVisitor(Integer author, Integer party) {
      this.author = author;
      this.party = party;
    }
    
    @Override
    public void setCountHint(int hint) {
    }
    
    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj, author, party);
      } catch (Exception e) {
        Log.d(TAG, "acceptTrack()", e);
      }
    }
  }
}
