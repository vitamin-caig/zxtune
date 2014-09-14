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

import android.content.Context;
import android.util.Log;
import app.zxtune.fs.VfsCache;

final class CachingCatalog extends Catalog {
  
  private final static String TAG = CachingCatalog.class.getName(); 
  private final static String CACHE_DIR_NAME = "www.zxart.ee";
  
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
    final int cached = db.queryAuthors(visitor, id);
    if (0 == cached) {
      Log.d(TAG, "Authors cache is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthors(new CachingAuthorsVisitor(), null);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
      db.queryAuthors(visitor, id);
    }
  }

  @Override
  public void queryAuthorTracks(TracksVisitor visitor, Integer id, Integer author)
      throws IOException {
    final int cached = db.queryAuthorTracks(visitor, id, author);
    if (0 == cached) {
      Log.d(TAG, "Tracks cache is empty for id=" + id + " author=" + author);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthorTracks(new CachingTracksVisitor(author, null), null, author);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
      db.queryAuthorTracks(visitor, id, author);
    }
  }

  @Override
  public void queryParties(PartiesVisitor visitor, Integer id) throws IOException {
    final int cached = db.queryParties(visitor, id);
    if (0 == cached) {
      Log.d(TAG, "Parties cache is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        //query all
        remote.queryParties(new CachingPartiesVisitor(), null);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
      db.queryParties(visitor, id);
    }
  }

  @Override
  public void queryPartyTracks(TracksVisitor visitor, Integer id, Integer party) throws IOException {
    final int cached = db.queryPartyTracks(visitor, id, party);
    if (0 == cached) {
      Log.d(TAG, "Tracks cache is empty for id=" + id + " party=" + party);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryPartyTracks(new CachingTracksVisitor(null, party), null, party);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
      db.queryPartyTracks(visitor, id, party);
    }
  }

  @Override
  public void queryTopTracks(TracksVisitor visitor, Integer id, int limit) throws IOException {
    Log.d(TAG, "Try to query top tracks from network");
    final Database.Transaction transaction = db.startTransaction();
    try {
      remote.queryTopTracks(new CachingTracksVisitor(null, null), id, limit);
      transaction.succeed();
    } catch (IOException e) {
      Log.d(TAG, "Failed", e);
    } finally {
      transaction.finish();
    }
    db.queryTopTracks(visitor, id, 100);
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
