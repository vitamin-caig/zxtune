package app.zxtune.fs.httpdir;

import android.content.Context;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.cache.CacheDir;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FileTree;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;

final class CachingCatalog extends Catalog {

  private final static TimeStamp DIR_TTL = TimeStamp.createFrom(1, TimeUnit.DAYS);

  private final String tag;
  private final RemoteCatalog remote;
  private final FileTree db;
  private final CacheDir cache;
  private final CommandExecutor executor;

  CachingCatalog(Context ctx, RemoteCatalog remote, CacheDir cache, String id) {
    this.tag = CachingCatalog.class.getName() + "@" + id;
    this.remote = remote;
    this.db = new FileTree(ctx, id);
    this.cache = cache.createNested(id);
    this.executor = new CommandExecutor(id);
  }

  @Nullable
  final File getFileCache(Path path) {
    return cache.find(path.getLocalId());
  }

  @Override
  public void parseDir(final Path path, final DirVisitor visitor) throws IOException {
    final String dirName = path.getLocalId();
    executor.executeQuery(new QueryCommand() {

      @Override
      public String getScope() {
        return "dir";
      }

      @Override
      public Timestamps.Lifetime getLifetime() {
        return db.getDirLifetime(dirName, DIR_TTL);
      }

      @Override
      public Transaction startTransaction() {
        return db.startTransaction();
      }

      @Override
      public void updateCache() throws IOException {
        final ArrayList<FileTree.Entry> entries = new ArrayList<>(100);
        remote.parseDir(path, new DirVisitor() {
          @Override
          public void acceptDir(String name, String desc) {
            entries.add(new FileTree.Entry(name, desc, null));
          }

          @Override
          public void acceptFile(String name, String descr, String size) {
            entries.add(new FileTree.Entry(name, descr, size));
          }
        });
        db.add(dirName, entries);
      }

      @Override
      public boolean queryFromCache() {
        final List<FileTree.Entry> entries = db.find(dirName);
        if (entries != null) {
          for (FileTree.Entry e : entries) {
            if (e.isDir()) {
              visitor.acceptDir(e.name, e.descr);
            } else {
              visitor.acceptFile(e.name, e.descr, e.size);
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
