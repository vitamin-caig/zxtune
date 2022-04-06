package app.zxtune.fs;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Comparator;

import app.zxtune.Log;
import app.zxtune.core.Identifier;
import app.zxtune.playlist.AylIterator;
import app.zxtune.playlist.ReferencesIterator.Entry;
import app.zxtune.playlist.xspf.XspfIterator;

final class VfsPlaylistDir implements VfsDir {

  private final static String TAG = VfsPlaylistDir.class.getName();

  private static final String TYPE_AYL = ".ayl";
  private static final String TYPE_XSPF = ".xspf";

  private final VfsFile file;
  private final ArrayList<Entry> entries;

  @Nullable
  static VfsDir resolveAsPlaylist(VfsFile file) {
    final Uri uri = file.getUri();
    final String filename = uri.getLastPathSegment();
    if (filename == null) {
      return null;
    }
    try {
      if (filename.endsWith(TYPE_AYL)) {
        return new VfsPlaylistDir(file, AylIterator.parse(Vfs.openStream(file)));
      } else if (filename.endsWith(TYPE_XSPF) || VfsRootPlaylists.SCHEME.equals(uri.getScheme())) {
        return new VfsPlaylistDir(file, XspfIterator.parse(Vfs.openStream(file)));
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to parse playlist");
    }
    return null;
  }

  static boolean maybePlaylist(Uri uri) {
    final String filename = uri.getLastPathSegment();
    if (filename == null) {
      return false;
    } else {
      return filename.endsWith(TYPE_AYL) || filename.endsWith(TYPE_XSPF) || VfsRootPlaylists.SCHEME.equals(uri.getScheme());
    }
  }

  private VfsPlaylistDir(VfsFile file, ArrayList<Entry> entries) {
    this.file = file;
    this.entries = entries;
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

  @Nullable
  @Override
  public VfsObject getParent() {
    return file.getParent();
  }

  @Nullable
  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.COMPARATOR.equals(id)) {
      return EntriesComparator.instance();
    } else {
      return null;
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    visitor.onItemsCount(entries.size());
    for (int idx = 0, lim = entries.size(); idx < lim; ++idx) {
      visitor.onFile(new PlaylistEntryFile(this, entries.get(idx), idx));
    }
  }

  private static class PlaylistEntryFile extends StubObject implements VfsFile {

    private final VfsDir parent;
    private final Entry entry;
    private final Identifier id;
    private final int index;

    PlaylistEntryFile(VfsDir parent, Entry entry, int index) {
      this.parent = parent;
      this.entry = entry;
      this.id = Identifier.parse(entry.location);
      this.index = index;
    }

    @Override
    public Uri getUri() {
      return id.getFullLocation();
    }

    @Override
    public String getName() {
      return entry.title.length() != 0 ? entry.title : id.getDisplayFilename();
    }

    @Override
    public String getDescription() {
      return entry.author;
    }

    @Nullable
    @Override
    public VfsObject getParent() {
      return parent;
    }

    @Override
    public String getSize() {
      return entry.duration.toString();
    }
  }

  private static class EntriesComparator implements Comparator<VfsObject> {

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      final int lhIndex = ((PlaylistEntryFile) lh).index;
      final int rhIndex = ((PlaylistEntryFile) rh).index;
      return lhIndex < rhIndex ? -1 : +1;
    }

    static Comparator<VfsObject> instance() {
      return Holder.INSTANCE;
    }

    //onDemand holder idiom
    private static class Holder {
      public static final EntriesComparator INSTANCE = new EntriesComparator();
    }
  }
}
