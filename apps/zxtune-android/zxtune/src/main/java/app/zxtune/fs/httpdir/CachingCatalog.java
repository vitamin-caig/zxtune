package app.zxtune.fs.httpdir;

import android.content.Context;

import androidx.annotation.VisibleForTesting;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.CommandExecutor;
import app.zxtune.fs.dbhelpers.FileTree;
import app.zxtune.fs.dbhelpers.QueryCommand;
import app.zxtune.fs.dbhelpers.Timestamps;

final class CachingCatalog implements Catalog {

  private final static TimeStamp DIR_TTL = TimeStamp.fromDays(1);

  private final RemoteCatalog remote;
  private final FileTree db;
  private final CommandExecutor executor;

  CachingCatalog(Context ctx, RemoteCatalog remote, String id) {
    this(remote, new FileTree(ctx, id), id);
  }

  @VisibleForTesting
  CachingCatalog(RemoteCatalog remote, FileTree db, String id) {
    this.remote = remote;
    this.db = db;
    this.executor = new CommandExecutor(id);
  }

  @Override
  public void parseDir(final Path path, final DirVisitor visitor) throws IOException {
    final String dirName = path.getLocalId();
    executor.executeQuery("dir", new QueryCommand() {

      private final Timestamps.Lifetime lifetime = db.getDirLifetime(dirName, DIR_TTL);

      @Override
      public boolean isCacheExpired() {
        return lifetime.isExpired();
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
        db.runInTransaction(() -> {
          db.add(dirName, entries);
          lifetime.update();
        });
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
