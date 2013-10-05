/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.util.Log;

/**
 * 
 */
final class CachingCatalog extends Catalog {
  
  private final static String TAG = CachingCatalog.class.getName();
  
  private final static String CACHE_DIR_NAME = "www.zxtunes.com";

  private final String cacheDir;
  private final Catalog remote;
  private final Database db;
  
  /**
   * 
   */
  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = context.getCacheDir().getAbsolutePath() + File.separator + CACHE_DIR_NAME + File.separator;
    this.remote = remote;
    this.db = db;
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor, Integer id) {
    final CountingAuthorsVisitor count = new CountingAuthorsVisitor(visitor);
    db.queryAuthors(count, id);
    if (0 == count.getAuthors()) {
      Log.d(TAG, "Authors cache is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthors(new CachingVisitor(visitor), id);
        transaction.succeed();
      } finally {
        transaction.finish();
      }
    }
  }
  
  @Override
  public void queryTracks(TracksVisitor visitor, Integer id, Integer author, Integer date) {
    remote.queryTracks(visitor, id, author, date);
  }
  
  @Override
  public byte[] getTrackContent(int id) throws IOException {
    final File cache = new File(cacheDir + Integer.toString(id));
    try {
      if (cache.exists()) {
        return readFrom(cache);
      }
    } catch (IOException e) {
    }
    final byte[] content = remote.getTrackContent(id);
    writeTo(cache, content);
    return content;
  }
  
  private byte[] readFrom(File file) throws IOException {
    final FileInputStream stream = new FileInputStream(file);
    try {
      final int len = (int) file.length();
      final byte[] content = new byte[len];
      stream.read(content, 0, len);
      return content;
    } finally {
      stream.close();
    }
  }
  
  private void writeTo(File file, byte[] data) {
    try {
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        stream.write(data);
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "writeTo " + file.getAbsolutePath(), e);
      file.delete();
    }
  }
  
  private static class CountingAuthorsVisitor implements AuthorsVisitor {

    private AuthorsVisitor delegate;
    private int authorsCount;
    
    CountingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
      this.authorsCount = 0;
    }
    
    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      ++authorsCount;
    }
    
    final int getAuthors() {
      return authorsCount;
    }
  }
  
  private class CachingVisitor implements AuthorsVisitor {
    
    private AuthorsVisitor delegate;
    
    CachingVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, "onAuthor()", e);
      }
    }
  }
}
