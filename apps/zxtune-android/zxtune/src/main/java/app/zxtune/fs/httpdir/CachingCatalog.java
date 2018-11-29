package app.zxtune.fs.httpdir;

import android.content.Context;
import android.support.annotation.NonNull;
import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.*;
import app.zxtune.fs.http.HttpObject;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.concurrent.TimeUnit;

final class CachingCatalog extends Catalog {

  private final static TimeStamp DIR_TTL = TimeStamp.createFrom(1, TimeUnit.DAYS);

  private final String tag;
  private final RemoteCatalog remote;
  private final FileTree db;
  private final CacheDir cache;
  private final CommandExecutor executor;

  CachingCatalog(Context ctx, RemoteCatalog remote, CacheDir cache, String id) throws IOException {
    this.tag = CachingCatalog.class.getName() + "@" + id;
    this.remote = remote;
    this.db = new FileTree(ctx, id);
    this.cache = cache.createNested(id);
    this.executor = new CommandExecutor(id);
  }

  @NonNull
  @Override
  public ByteBuffer getFileContent(final Path path) throws IOException {
    return executor.executeDownloadCommand(new DownloadCommand() {
      @NonNull
      @Override
      public File getCache() throws IOException {
        return cache.findOrCreate(path.getLocalId());
      }

      @NonNull
      @Override
      public HttpObject getRemote() throws IOException {
        return remote.getFileObject(path);
      }
    });
  }

  @Override
  public void parseDir(final Path path, final DirVisitor visitor) throws IOException {
    final String dirName = path.getLocalId();
    executor.executeQuery("dir", new QueryCommand() {
      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getDirLifetime(dirName, DIR_TTL);
      }

      @Override
      public Transaction startTransaction() throws IOException {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        final HashMap<String, String> entries = new HashMap<>();
        remote.parseDir(path, new DirVisitor() {
          @Override
          public void acceptDir(String name) {
            entries.put(name, null);
          }

          @Override
          public void acceptFile(String name, String size) {
            entries.put(name, size);
          }
        });
        db.add(dirName, entries);
      }

      @Override
      public boolean queryFromCache() {
        final HashMap<String, String> entries = db.find(dirName);
        if (entries != null) {
          for (HashMap.Entry<String, String> ent : entries.entrySet()) {
            final String name = ent.getKey();
            final String size = ent.getValue();
            if (size == null) {
              visitor.acceptDir(name);
            } else {
              visitor.acceptFile(name, size);
            }
          }
          return true;
        } else {
          return false;
        }
      }
    });
  }
}
