/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import app.zxtune.Log;
import app.zxtune.MainApplication;
import app.zxtune.core.Identifier;
import app.zxtune.fs.VfsDir.Visitor;
import app.zxtune.fs.archives.Archive;
import app.zxtune.fs.archives.ArchivesService;
import app.zxtune.fs.archives.DirEntry;
import app.zxtune.fs.archives.Entry;
import app.zxtune.fs.archives.Track;
import app.zxtune.utils.ProgressCallback;
import app.zxtune.utils.StubProgressCallback;

public final class VfsArchive {

  private static final String TAG = VfsArchive.class.getName();

  private static final int ARCHIVE_ENTRY_TRACKS_COUNT = 1;
  private static final int PLAYLIST_TRACKS_COUNT = 2;

  private final ArchivesService service;

  private VfsArchive() {
    this.service = new ArchivesService(MainApplication.getGlobalContext());
  }

  public static Integer[] getModulesCount(Uri[] uris) {
    final Integer[] result = new Integer[uris.length];
    final HashMap<Uri, Integer> positions = new HashMap<>();
    final ArrayList<Uri> query = new ArrayList<>();
    for (int i = 0; i < uris.length; ++i) {
      final Uri uri = uris[i];
      if (isArchived(uri)) {
        result[i] = ARCHIVE_ENTRY_TRACKS_COUNT;
      } else if (VfsPlaylistDir.maybePlaylist(uri)) {
        result[i] = PLAYLIST_TRACKS_COUNT;
      } else {
        positions.put(uri, i);
        query.add(uri);
      }
    }
    for (Archive arch : Holder.INSTANCE.service.findArchives(query)) {
      final Integer pos = positions.get(arch.path);
      if (pos != null) {
        result[pos] = arch.modules;
      }
    }
    return result;
  }

  private static boolean isArchived(Uri uri) {
    final Identifier id = new Identifier(uri);
    return !id.getSubPath().isEmpty();
  }

  /*
   * @return
   *
   * VfsDir - browsable archive
   * VfsFile - single file to play (or no files to play)
   * null - unknown status
   */
  @Nullable
  static VfsObject browseCached(VfsFile file) {
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
    final Archive arc = service.findArchive(file.getUri());
    return browseCachedFile(file, arc);
  }

  @Nullable
  private VfsObject browseCachedFile(VfsFile file, @Nullable Archive arc) {
    if (arc == null) {
      Log.d(TAG, "Unknown archive %s", file.getUri());
      return null;
    } else if (arc.modules < 2) {
      Log.d(TAG, "Too few modules in archive %s", file.getUri());
      return file;
    }
    return new ArchiveRoot(file);
  }

  /*
   * @return
   *
   * VfsDir - browsable archive
   * VfsFile - single file to play
   * null - nothing to play
   */
  @Nullable
  public static VfsObject browse(VfsFile file) {
    return Holder.INSTANCE.browseFile(file, StubProgressCallback.instance());
  }

  @Nullable
  private VfsObject browseFile(VfsFile file, ProgressCallback cb) {
    final VfsDir asPlaylist = VfsPlaylistDir.resolveAsPlaylist(file);
    if (asPlaylist != null) {
      return asPlaylist;
    }
    try {
      final Archive arc = service.analyzeArchive(file, cb);
      return browseCachedFile(file, arc);
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to analyze archive");
      return null;
    }
  }

  @Nullable
  public static VfsObject resolve(Uri uri) throws IOException {
    return Holder.INSTANCE.resolveUri(uri, null);
  }

  @Nullable
  public static VfsObject resolveForced(Uri uri, ProgressCallback cb) throws IOException {
    return Holder.INSTANCE.resolveUri(uri, cb);
  }

  // TODO: clarify forced == cb != 0 semantic
  @Nullable
  private VfsObject resolveUri(Uri uri, @Nullable ProgressCallback cb) throws IOException {
    final Identifier id = new Identifier(uri);
    final String subpath = id.getSubPath();
    if (TextUtils.isEmpty(subpath)) {
      return resolveFileUri(uri, cb);
    } else {
      return resolveArchiveUri(uri, cb);
    }
  }

  @Nullable
  private VfsObject resolveFileUri(Uri uri, @Nullable ProgressCallback cb) throws IOException {
    final VfsObject obj = Vfs.resolve(uri);
    if (obj instanceof VfsFile) {
      final VfsObject cached = browseCachedFile((VfsFile) obj);
      if (cached != null) {
        return cached;
      } else if (cb != null) {
        return browseFile((VfsFile) obj, cb);
      }
    }
    return obj;
  }

  private VfsObject resolveArchiveUri(Uri uri, @Nullable ProgressCallback cb) throws IOException {
    final Entry entry = service.resolve(uri);
    if (entry != null) {
      if (entry.track != null) {
        return new ArchiveFile(null, entry.dirEntry, entry.track);
      } else {
        return new ArchiveDir(null, entry.dirEntry);
      }
    }
    if (cb != null) {
      final VfsObject real = Vfs.resolve(uri.buildUpon().fragment("").build());
      if (real instanceof VfsFile) {
        if (browseFile((VfsFile) real, cb) != null) {
          return resolveArchiveUri(uri, null);
        }
      }
    }
    throw new IOException("No archive found");
  }

  private void listArchive(final VfsObject parent, final Visitor visitor) {
    service.listDir(parent.getUri(), new ArchivesService.ListingCallback() {
      @Override
      public void onItemsCount(int hint) {
        visitor.onItemsCount(hint);
      }

      @Override
      public void onEntry(Entry entry) {
        if (entry.track != null) {
          visitor.onFile(new ArchiveFile(parent, entry.dirEntry, entry.track));
        } else {
          visitor.onDir(new ArchiveDir(parent, entry.dirEntry));
        }
      }
    });
  }

  private class ArchiveRoot implements VfsDir, VfsFile {

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
    public String getDescription() {
      return file.getDescription();
    }

    @Override
    public VfsObject getParent() {
      return file.getParent();
    }

    @Override
    public Object getExtension(String id) {
      return file.getExtension(id);
    }

    @Override
    public String getSize() {
      return file.getSize();
    }

    @Override
    public void enumerate(Visitor visitor) {
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
          parent = resolveUri(entry.parent.getFullLocation(), null);
        }
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to resolve");
      }
      return parent;
    }
  }

  private class ArchiveDir extends ArchiveEntry implements VfsDir {

    ArchiveDir(@Nullable VfsObject parent, DirEntry entry) {
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
    public void enumerate(Visitor visitor) {
      listArchive(this, visitor);
    }
  }

  private class ArchiveFile extends ArchiveEntry implements VfsFile {

    private final Track track;

    ArchiveFile(@Nullable VfsObject parent, DirEntry entry, Track track) {
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

    @Override
    public String getSize() {
      return track.duration.toString();
    }
  }

  static boolean checkIfArchive(VfsDir dir) {
    return dir instanceof ArchiveRoot || dir instanceof ArchiveDir;
  }

  private static class Holder {
    public static final VfsArchive INSTANCE = new VfsArchive();
  }
}
