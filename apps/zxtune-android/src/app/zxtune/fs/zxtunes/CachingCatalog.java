/**
 *
 * @file
 *
 * @brief Caching catalog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxtunes;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import app.zxtune.Log;
import app.zxtune.fs.VfsCache;

final class CachingCatalog extends Catalog {
  
  private final static String TAG = CachingCatalog.class.getName();
  
  private final static String CACHE_DIR_NAME = "www.zxtunes.com";

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
    final CountingAuthorsVisitor count = new CountingAuthorsVisitor(visitor);
    db.queryAuthors(count, id);
    if (0 == count.get()) {
      Log.d(TAG, "Authors cache is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        //query all
        remote.queryAuthors(new CachingAuthorsVisitor(), null);
        transaction.succeed();
        Log.d(TAG, "Cached %d authors", count.get());
      } finally {
        transaction.finish();
      }
      db.queryAuthors(visitor, id);
    }
  }
  
  @Override
  public void queryTracks(TracksVisitor visitor, Integer id, Integer author) throws IOException {
    //TODO: make another way to check if cache is filled
    final CountingTracksVisitor count = new CountingTracksVisitor(visitor);
    db.queryTracks(count, id, author);
    if (0 == count.get()) {
      Log.d(TAG, "Tracks cache is empty for id=%s author=%s", id, author);
      final Database.Transaction transaction = db.startTransaction();
      try {
        //query all
        remote.queryTracks(new CachingTracksVisitor(author), null, author);
        transaction.succeed();
        Log.d(TAG, "Cached %d tracks", count.get());
      } finally {
        transaction.finish();
      }
      db.queryTracks(visitor, id, author);
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
  
  private static class CountingAuthorsVisitor implements AuthorsVisitor {

    private final AuthorsVisitor delegate;
    private int count;
    
    CountingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }
    
    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      ++count;
    }
    
    final int get() {
      return count;
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
        Log.d(TAG, e, "acceptAuthor()");
      }
    }
  }
  
  private static class CountingTracksVisitor implements TracksVisitor {
    
    private final TracksVisitor delegate;
    private int count;
    
    CountingTracksVisitor(TracksVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void setCountHint(int hint) {
      delegate.setCountHint(hint);
    }
    
    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      ++count;
    }
    
    final int get() {
      return count;
    }
  }
  
  private class CachingTracksVisitor implements TracksVisitor {
    
    private final Integer author;
    
    CachingTracksVisitor(Integer author) {
      this.author = author;
    }

    @Override
    public void setCountHint(int hint) {
    }
    
    @Override
    public void accept(Track obj) {
      try {
        db.addTrack(obj, author);
      } catch (Exception e) {
        Log.d(TAG, e, "addTrack()");
      }
    }
  }
}
