/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import app.zxtune.core.Identifier;
import app.zxtune.Log;
import app.zxtune.MainApplication;
import app.zxtune.fs.VfsDir.Visitor;
import app.zxtune.fs.archives.Archive;
import app.zxtune.fs.archives.DirEntry;
import app.zxtune.fs.archives.Entry;
import app.zxtune.fs.archives.Query;
import app.zxtune.fs.archives.Track;

import java.io.IOException;
import java.nio.ByteBuffer;

public final class VfsArchive {

  private static final String TAG = VfsArchive.class.getName();

  private final ContentResolver resolver;

  private VfsArchive() {
    this.resolver = MainApplication.getInstance().getContentResolver();
  }

  /*
   * @return
   *
   * VfsDir - browsable archive
   * VfsFile - single file to play (or no files to play)
   * null - unknown status
   */
  @Nullable
  public static VfsObject browseCached(VfsFile file) {
    if (file instanceof ArchiveFile) {
      return file;
    }
    return Holder.INSTANCE.browseCachedFile(file);
  }

  @Nullable
  private VfsObject browseCachedFile(VfsFile file) {
    final VfsDir asPlaylist = VfsPlaylistDir.resolveAsPlaylist(file);
    if (asPlaylist != null) {
      return asPlaylist;
    }
    final Uri uri = file.getUri();
    final Integer modulesInArchive = getModulesInArchive(uri);
    if (modulesInArchive == null) {
      Log.d(TAG, "Unknown archive %s", uri);
      return null;
    } else if (modulesInArchive < 2) {
      Log.d(TAG, "Too few modules in archive %s", uri);
      return file;
    }
    return new ArchiveRoot(file);
  }

  @Nullable
  private Integer getModulesInArchive(@NonNull Uri uri) {
    final Cursor cursor = resolver.query(Query.archiveUriFor(uri), null, null, null, null);
    if (cursor == null) {
      return null;
    }
    try {
      if (cursor.moveToFirst()) {
        final Archive arch = Archive.fromCursor(cursor);
        return arch.modules;
      }
    } finally {
      cursor.close();
    }
    return null;
  }

  /*
   * @return
   *
   * VfsDir - browsable archive
   * VfsFile - single file to play
   * null - nothing to play
   */
  @Nullable
  public static VfsObject browse(@NonNull VfsFile file) {
    return Holder.INSTANCE.browseFile(file);
  }

  private VfsObject browseFile(VfsFile file) {
    final VfsDir asPlaylist = VfsPlaylistDir.resolveAsPlaylist(file);
    if (asPlaylist != null) {
      return asPlaylist;
    }
    final Uri uri = file.getUri();
    final Uri newUri = resolver.insert(Query.archiveUriFor(uri), new ContentValues());
    if (newUri == null) {
      return null;
    } else {
      return browseCachedFile(file);
    }
  }

  @Nullable
  public static VfsObject resolve(@NonNull Uri uri) throws IOException {
    return Holder.INSTANCE.resolveUri(uri, false);
  }

  @Nullable
  public static VfsObject resolveForced(@NonNull Uri uri) throws IOException {
    return Holder.INSTANCE.resolveUri(uri, true);
  }

  @Nullable
  private VfsObject resolveUri(@NonNull Uri uri, boolean forceAnalyze) throws IOException {
    final Identifier id = new Identifier(uri);
    final String subpath = id.getSubpath();
    if (TextUtils.isEmpty(subpath)) {
      return resolveFileUri(uri, forceAnalyze);
    } else {
      return resolveArchiveUri(uri, forceAnalyze);
    }
  }

  @Nullable
  private VfsObject resolveFileUri(Uri uri, boolean forceAnalyze) throws IOException {
    final VfsObject obj = Vfs.resolve(uri);
    if (obj instanceof VfsFile) {
      final VfsObject cached = browseCachedFile((VfsFile) obj);
      if (cached != null) {
        return cached;
      } else if (forceAnalyze) {
        return browseFile((VfsFile) obj);
      }
    }
    return obj;
  }

  private VfsObject resolveArchiveUri(@NonNull Uri uri, boolean forceAnalyze) throws IOException {
    final Cursor cursor = resolver.query(Query.infoUriFor(uri), null, null, null, null);
    if (cursor == null) {
      throw new IOException("Failed to query archive info");
    }
    try {
      if (cursor.moveToFirst()) {
        final Entry entry = Entry.fromCursor(cursor);
        if (entry.track != null) {
          return new ArchiveFile(null, entry.dirEntry, entry.track);
        } else {
          return new ArchiveDir(null, entry.dirEntry);
        }
      }
    } finally {
      cursor.close();
    }
    if (forceAnalyze) {
      final VfsObject real = Vfs.resolve(uri.buildUpon().fragment("").build());
      if (real instanceof VfsFile) {
        if (browseFile((VfsFile) real) != null) {
          return resolveArchiveUri(uri, false);
        }
      }
    }
    throw new IOException("No archive found");
  }

  private void listArchive(VfsObject parent, @NonNull Visitor visitor) throws IOException {
    final Cursor cursor = resolver.query(Query.listDirUriFor(parent.getUri()), null, null, null, null);
    if (cursor == null) {
      throw new IOException("Failed query");
    }
    try {
      visitor.onItemsCount(cursor.getCount());
      while (cursor.moveToNext()) {
        final Entry entry = Entry.fromCursor(cursor);
        if (entry.track != null) {
          visitor.onFile(new ArchiveFile(parent, entry.dirEntry, entry.track));
        } else {
          visitor.onDir(new ArchiveDir(parent, entry.dirEntry));
        }
      }
    } finally {
      cursor.close();
    }
  }

  private class ArchiveRoot extends StubObject implements VfsDir {

    private final VfsFile file;

    ArchiveRoot(VfsFile file) {
      this.file = file;
    }

    @Override
    public Uri getUri() {
      return file.getUri();
    }

    @Override
    public String getName() {
      return file.getName();
    }

    @Override
    public VfsObject getParent() {
      return file.getParent();
    }

    @Override
    public void enumerate(@NonNull Visitor visitor) throws IOException {
      listArchive(this, visitor);
    }
  }

  private abstract class ArchiveEntry extends StubObject {

    @Nullable
    private VfsObject parent;
    final DirEntry entry;

    ArchiveEntry(@Nullable VfsObject parent, DirEntry entry) {
      this.parent = parent;
      this.entry = entry;
    }

    @Override
    public VfsObject getParent() {
      try {
        if (parent == null) {
          parent = resolveUri(entry.parent.getFullLocation(), false);
        }
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to resolve");
      }
      return parent;
    }
  }

  private class ArchiveDir extends ArchiveEntry implements VfsDir {

    ArchiveDir(VfsObject parent, DirEntry entry) {
      super(parent, entry);
    }

    @Override
    public Uri getUri() {
      return entry.path.getFullLocation();
    }

    @Override
    public String getName() {
      return entry.filename;
    }

    @Override
    public void enumerate(@NonNull Visitor visitor) throws IOException {
      listArchive(this, visitor);
    }
  }

  private class ArchiveFile extends ArchiveEntry implements VfsFile {

    private final Track track;

    ArchiveFile(VfsObject parent, DirEntry entry, Track track) {
      super(parent, entry);
      this.track = track;
    }

    @Override
    public Uri getUri() {
      return track.path;
    }

    @Override
    public String getName() {
      return track.filename;
    }

    @Override
    public String getDescription() {
      return track.description;
    }

    @NonNull
    @Override
    public String getSize() {
      return track.duration.toString();
    }

    @NonNull
    @Override
    public ByteBuffer getContent() throws IOException {
      throw new IOException("Should not be called");
    }
  }

  public static boolean checkIfArchive(VfsDir dir) {
    return dir instanceof ArchiveRoot || dir instanceof ArchiveDir;
  }

  private static class Holder {
    public static final VfsArchive INSTANCE = new VfsArchive();
  }
}
